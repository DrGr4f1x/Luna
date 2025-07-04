//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#include "Stdafx.h"

#include "DeviceVK.h"

#include "FileSystem.h"

#include "Graphics\CommandContext.h"
#include "Graphics\Shader.h"

#include "ColorBufferVK.h"
#include "DepthBufferVK.h"
#include "DescriptorSetVK.h"
#include "GpuBufferVK.h"
#include "PipelineStateVK.h"
#include "RootSignatureVK.h"
#include "SamplerVK.h"
#include "TextureVK.h"

using namespace std;


namespace Luna::VK
{

static Device* g_vulkanDevice{ nullptr };


inline VkDescriptorType RootParameterTypeToVulkanDescriptorType(RootParameterType rootParameterType)
{
	switch (rootParameterType)
	{
	case RootParameterType::RootCBV: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	case RootParameterType::RootSRV:
	case RootParameterType::RootUAV: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
	default:						 return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}
}


pair<string, bool> GetShaderFilenameWithExtension(const string& shaderFilename)
{
	auto fileSystem = GetFileSystem();

	string shaderFileWithExtension = shaderFilename;
	bool exists = false;

	// See if the filename already has an extension
	string extension = fileSystem->GetFileExtension(shaderFilename);
	if (!extension.empty())
	{
		exists = fileSystem->Exists(shaderFileWithExtension);
	}
	else
	{
		// Try .spirv extension
		shaderFileWithExtension = shaderFilename + ".spirv";
		exists = fileSystem->Exists(shaderFileWithExtension);
	}

	return make_pair(shaderFileWithExtension, exists);
}


Shader* LoadShader(ShaderType type, const ShaderNameAndEntry& shaderNameAndEntry)
{
	auto [shaderFilenameWithExtension, exists] = GetShaderFilenameWithExtension(shaderNameAndEntry.shaderFile);

	if (!exists)
	{
		return nullptr;
	}

	ShaderDesc shaderDesc{
		.filenameWithExtension = shaderFilenameWithExtension,
		.entry = shaderNameAndEntry.entry,
		.type = type
	};

	return Shader::Load(shaderDesc);
}


void FillShaderStageCreateInfo(VkPipelineShaderStageCreateInfo& createInfo, VkShaderModule shaderModule, const Shader* shader)
{
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.stage = ShaderTypeToVulkan(shader->GetShaderType());
	createInfo.pName = shader->GetEntry().c_str();
	createInfo.module = shaderModule;
	createInfo.pSpecializationInfo = nullptr;
}


Device::Device(CVkDevice* device, CVmaAllocator* allocator)
	: m_device{ device }
	, m_allocator{ allocator }
{
	m_pipelineCache = CreatePipelineCache();

	assert(g_vulkanDevice == nullptr);
	g_vulkanDevice = this;
}


ColorBufferPtr Device::CreateColorBuffer(const ColorBufferDesc& colorBufferDesc)
{
	// Create image
	ImageDesc imageDesc{
		.name				= colorBufferDesc.name,
		.width				= colorBufferDesc.width,
		.height				= colorBufferDesc.height,
		.arraySizeOrDepth	= colorBufferDesc.arraySizeOrDepth,
		.format				= colorBufferDesc.format,
		.numMips			= colorBufferDesc.numMips,
		.numSamples			= colorBufferDesc.numSamples,
		.resourceType		= colorBufferDesc.resourceType,
		.imageUsage			= GpuImageUsage::ColorBuffer,
		.memoryAccess		= MemoryAccess::GpuReadWrite
	};

	if (HasFlag(colorBufferDesc.resourceType, ResourceType::Texture3D))
	{
		imageDesc.SetNumMips(1);
		imageDesc.SetDepth(colorBufferDesc.arraySizeOrDepth);
	}
	else if (HasAnyFlag(colorBufferDesc.resourceType, ResourceType::Texture2D_Type))
	{
		if (HasAnyFlag(colorBufferDesc.resourceType, ResourceType::TextureArray_Type))
		{
			imageDesc.SetResourceType(colorBufferDesc.numSamples == 1 ? ResourceType::Texture2D_Array : ResourceType::Texture2DMS_Array);
			imageDesc.SetArraySize(colorBufferDesc.arraySizeOrDepth);
		}
		else
		{
			imageDesc.SetResourceType(colorBufferDesc.numSamples == 1 ? ResourceType::Texture2D : ResourceType::Texture2DMS_Array);
		}
	}

	auto image = CreateImage(imageDesc);

	// Render target view
	ImageViewDesc imageViewDesc{
		.image				= image.get(),
		.name				= colorBufferDesc.name,
		.resourceType		= ResourceType::Texture2D,
		.imageUsage			= GpuImageUsage::RenderTarget,
		.format				= colorBufferDesc.format,
		.imageAspect		= ImageAspect::Color,
		.baseMipLevel		= 0,
		.mipCount			= colorBufferDesc.numMips,
		.baseArraySlice		= 0,
		.arraySize			= colorBufferDesc.arraySizeOrDepth
	};
	auto imageViewRtv = CreateImageView(imageViewDesc);

	// Shader resource view
	imageViewDesc.SetImageUsage(GpuImageUsage::ShaderResource);
	auto imageViewSrv = CreateImageView(imageViewDesc);

	// Descriptors
	VkDescriptorImageInfo imageInfoSrv{
		.sampler		= VK_NULL_HANDLE,
		.imageView		= *imageViewSrv,
		.imageLayout	= GetImageLayout(ResourceState::ShaderResource)
	};
	VkDescriptorImageInfo imageInfoUav{
		.sampler		= VK_NULL_HANDLE,
		.imageView		= *imageViewSrv,
		.imageLayout	= GetImageLayout(ResourceState::UnorderedAccess)
	};

	auto colorBuffer = std::make_shared<Luna::VK::ColorBuffer>();

	colorBuffer->m_device = this;
	colorBuffer->m_type = colorBufferDesc.resourceType;
	colorBuffer->m_usageState = ResourceState::Undefined;
	colorBuffer->m_width = colorBufferDesc.width;
	colorBuffer->m_height = colorBufferDesc.height;
	colorBuffer->m_arraySizeOrDepth = colorBufferDesc.arraySizeOrDepth;
	colorBuffer->m_numMips = colorBufferDesc.numMips;
	colorBuffer->m_numSamples = colorBufferDesc.numSamples;
	colorBuffer->m_format = colorBufferDesc.format;
	colorBuffer->m_dimension = ResourceTypeToTextureDimension(colorBuffer->m_type);
	colorBuffer->m_clearColor = colorBufferDesc.clearColor;
	colorBuffer->m_image = image;
	colorBuffer->m_imageViewRtv = imageViewRtv;
	colorBuffer->m_imageViewSrv = imageViewSrv;
	colorBuffer->m_imageInfoSrv = imageInfoSrv;
	colorBuffer->m_imageInfoUav = imageInfoUav;

	return colorBuffer;
}


DepthBufferPtr Device::CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc)
{
	// Create depth image
	ImageDesc imageDesc{
		.name				= depthBufferDesc.name,
		.width				= depthBufferDesc.width,
		.height				= depthBufferDesc.height,
		.arraySizeOrDepth	= depthBufferDesc.arraySizeOrDepth,
		.format				= depthBufferDesc.format,
		.numMips			= depthBufferDesc.numMips,
		.numSamples			= depthBufferDesc.numSamples,
		.resourceType		= depthBufferDesc.resourceType,
		.imageUsage			= GpuImageUsage::DepthBuffer,
		.memoryAccess		= MemoryAccess::GpuReadWrite
	};

	auto image = CreateImage(imageDesc);

	// Create image views and descriptors
	const bool bHasStencil = IsStencilFormat(depthBufferDesc.format);

	auto imageAspect = ImageAspect::Depth;
	if (bHasStencil)
	{
		imageAspect |= ImageAspect::Stencil;
	}

	ImageViewDesc imageViewDesc{
		.image				= image.get(),
		.name				= depthBufferDesc.name,
		.resourceType		= ResourceType::Texture2D,
		.imageUsage			= GpuImageUsage::DepthStencilTarget,
		.format				= depthBufferDesc.format,
		.imageAspect		= imageAspect,
		.baseMipLevel		= 0,
		.mipCount			= depthBufferDesc.numMips,
		.baseArraySlice		= 0,
		.arraySize			= depthBufferDesc.arraySizeOrDepth
	};

	auto imageViewDepthStencil = CreateImageView(imageViewDesc);
	wil::com_ptr<CVkImageView> imageViewDepthOnly;
	wil::com_ptr<CVkImageView> imageViewStencilOnly;

	if (bHasStencil)
	{
		imageViewDesc
			.SetName(format("{} Depth Image View", depthBufferDesc.name))
			.SetImageAspect(ImageAspect::Depth)
			.SetViewType(TextureSubresourceViewType::DepthOnly);

		imageViewDepthOnly = CreateImageView(imageViewDesc);

		imageViewDesc
			.SetName(format("{} Stencil Image View", depthBufferDesc.name))
			.SetImageAspect(ImageAspect::Stencil)
			.SetViewType(TextureSubresourceViewType::StencilOnly);

		imageViewStencilOnly = CreateImageView(imageViewDesc);
	}
	else
	{
		imageViewDepthOnly = imageViewDepthStencil;
		imageViewStencilOnly = imageViewDepthStencil;
	}

	auto imageInfoDepth = VkDescriptorImageInfo{
		.sampler		= VK_NULL_HANDLE,
		.imageView		= *imageViewDepthOnly,
		.imageLayout	= GetImageLayout(ResourceState::ShaderResource)
	};
	auto imageInfoStencil = VkDescriptorImageInfo{
		.sampler		= VK_NULL_HANDLE,
		.imageView		= *imageViewStencilOnly,
		.imageLayout	= GetImageLayout(ResourceState::ShaderResource)
	};

	auto depthBuffer = std::make_shared<Luna::VK::DepthBuffer>();

	depthBuffer->m_device = this;
	depthBuffer->m_type = depthBufferDesc.resourceType;
	depthBuffer->m_usageState = ResourceState::Undefined;
	depthBuffer->m_width = depthBufferDesc.width;
	depthBuffer->m_height = depthBufferDesc.height;
	depthBuffer->m_arraySizeOrDepth = depthBufferDesc.arraySizeOrDepth;
	depthBuffer->m_numMips = 1;
	depthBuffer->m_numSamples = depthBufferDesc.numSamples;
	depthBuffer->m_format = depthBufferDesc.format;
	depthBuffer->m_dimension = ResourceTypeToTextureDimension(depthBuffer->m_type);
	depthBuffer->m_clearDepth = depthBufferDesc.clearDepth;
	depthBuffer->m_clearStencil = depthBufferDesc.clearStencil;
	depthBuffer->m_image = image;
	depthBuffer->m_imageViewDepthStencil = imageViewDepthStencil;
	depthBuffer->m_imageViewDepthOnly = imageViewDepthOnly;
	depthBuffer->m_imageViewStencilOnly = imageViewStencilOnly;
	depthBuffer->m_imageInfoDepth = imageInfoDepth;
	depthBuffer->m_imageInfoStencil = imageInfoStencil;

	return depthBuffer;
}


GpuBufferPtr Device::CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc)
{
	constexpr VkBufferUsageFlags transferFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VkBufferCreateInfo bufferCreateInfo{
		.sType	= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size	= gpuBufferDesc.elementCount * gpuBufferDesc.elementSize,
		.usage	= GetBufferUsageFlags(gpuBufferDesc.resourceType) | transferFlags
	};

	VmaAllocationCreateInfo allocCreateInfo{};
	allocCreateInfo.flags = GetMemoryFlags(gpuBufferDesc.memoryAccess);
	allocCreateInfo.usage = GetMemoryUsage(gpuBufferDesc.memoryAccess);

	VkBuffer vkBuffer{ VK_NULL_HANDLE };
	VmaAllocation vmaBufferAllocation{ VK_NULL_HANDLE };

	auto res = vmaCreateBuffer(*m_allocator, &bufferCreateInfo, &allocCreateInfo, &vkBuffer, &vmaBufferAllocation, nullptr);
	assert(res == VK_SUCCESS);

	SetDebugName(*m_device, vkBuffer, gpuBufferDesc.name);

	auto buffer = Create<CVkBuffer>(m_device.get(), m_allocator.get(), vkBuffer, vmaBufferAllocation);

	wil::com_ptr<CVkBufferView> bufferView;
	if (gpuBufferDesc.resourceType == ResourceType::TypedBuffer)
	{
		VkBufferViewCreateInfo bufferViewCreateInfo{
			.sType		= VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
			.buffer		= vkBuffer,
			.format		= FormatToVulkan(gpuBufferDesc.format),
			.offset		= 0,
			.range		= VK_WHOLE_SIZE
		};
		VkBufferView vkBufferView{ VK_NULL_HANDLE };
		vkCreateBufferView(*m_device, &bufferViewCreateInfo, nullptr, &vkBufferView);
		bufferView = Create<CVkBufferView>(m_device.get(), vkBufferView);
	}

	auto gpuBuffer = std::make_shared<GpuBuffer>();

	gpuBuffer->m_device = this;
	gpuBuffer->m_type = gpuBufferDesc.resourceType;
	gpuBuffer->m_format = gpuBufferDesc.format;
	gpuBuffer->m_usageState = ResourceState::GenericRead;
	gpuBuffer->m_elementSize = gpuBufferDesc.elementSize;
	gpuBuffer->m_elementCount = gpuBufferDesc.elementCount;
	gpuBuffer->m_bufferSize = gpuBuffer->m_elementCount * gpuBuffer->m_elementSize;
	gpuBuffer->m_buffer = buffer;
	gpuBuffer->m_bufferView = bufferView;
	gpuBuffer->m_bufferInfo = { .buffer = vkBuffer, .offset = 0, .range = VK_WHOLE_SIZE };
	gpuBuffer->m_isCpuWriteable = HasFlag(gpuBufferDesc.memoryAccess, MemoryAccess::CpuWrite);

	if (gpuBufferDesc.initialData)
	{
		CommandContext::InitializeBuffer(gpuBuffer, gpuBufferDesc.initialData, gpuBuffer->GetBufferSize());
	}

	return gpuBuffer;
}


RootSignaturePtr Device::CreateRootSignature(const RootSignatureDesc& rootSignatureDesc)
{
	// Validate RootSignatureDesc
	if (!rootSignatureDesc.Validate())
	{
		LogError(LogVulkan) << "RootSignature is not valid!" << endl;
		return nullptr;
	}

	std::vector<VkDescriptorSetLayout> vkDescriptorSetLayouts;
	std::vector<wil::com_ptr<CVkDescriptorSetLayout>> descriptorSetLayouts;
	std::vector<VkPushConstantRange> vkPushConstantRanges;
	std::unordered_map<uint32_t, std::vector<DescriptorBindingDesc>> layoutBindingMap;
	std::unordered_map<uint32_t, uint32_t> rootParameterIndexToDescriptorSetMap;
	uint32_t pushConstantOffset{ 0 };

	size_t hashCode = Utility::g_hashStart;

	uint32_t rootParamIndex = 0;
	for (const auto& rootParameter : rootSignatureDesc.rootParameters)
	{
		std::vector<VkDescriptorSetLayoutBinding> vkLayoutBindings;
		std::vector<DescriptorBindingDesc> layoutBindingDescs;

		VkShaderStageFlags shaderStageFlags = ShaderStageToVulkan(rootParameter.shaderVisibility);

		bool createLayout = true;
		uint32_t offset = 0;

		if (rootParameter.parameterType == RootParameterType::RootConstants)
		{
			VkPushConstantRange& vkPushConstantRange = vkPushConstantRanges.emplace_back();
			vkPushConstantRange.offset = pushConstantOffset;
			vkPushConstantRange.size = rootParameter.num32BitConstants * 4;
			vkPushConstantRange.stageFlags = shaderStageFlags;
			pushConstantOffset += rootParameter.num32BitConstants * 4;

			hashCode = Utility::HashState(&vkPushConstantRange, 1, hashCode);

			createLayout = false;
		}
		else if (rootParameter.parameterType == RootParameterType::RootCBV ||
			rootParameter.parameterType == RootParameterType::RootSRV ||
			rootParameter.parameterType == RootParameterType::RootUAV)
		{
			VkDescriptorSetLayoutBinding& vkBinding = vkLayoutBindings.emplace_back();
			vkBinding.descriptorType = RootParameterTypeToVulkanDescriptorType(rootParameter.parameterType);

			vkBinding.binding = rootParameter.startRegister;
			vkBinding.descriptorCount = 1;
			vkBinding.stageFlags = shaderStageFlags;
			vkBinding.pImmutableSamplers = nullptr;

			hashCode = Utility::HashState(&vkBinding, 1, hashCode);

			DescriptorBindingDesc& bindingDesc = layoutBindingDescs.emplace_back();
			bindingDesc.descriptorType = vkBinding.descriptorType;
			bindingDesc.startSlot = rootParameter.startRegister;
			bindingDesc.numDescriptors = vkBinding.descriptorCount;
			bindingDesc.offset = offset;
		}
		else if (rootParameter.parameterType == RootParameterType::Table)
		{
			for (const auto& range : rootParameter.table)
			{
				VkDescriptorSetLayoutBinding& vkBinding = vkLayoutBindings.emplace_back();
				vkBinding.descriptorCount = range.numDescriptors;
				vkBinding.descriptorType = DescriptorTypeToVulkan(range.descriptorType);

				vkBinding.binding = range.startRegister;
				vkBinding.stageFlags = shaderStageFlags;
				vkBinding.pImmutableSamplers = nullptr;

				hashCode = Utility::HashState(&vkBinding, 1, hashCode);

				DescriptorBindingDesc& bindingDesc = layoutBindingDescs.emplace_back();
				bindingDesc.descriptorType = vkBinding.descriptorType;
				bindingDesc.startSlot = rootParameter.startRegister;
				bindingDesc.numDescriptors = vkBinding.descriptorCount;
				bindingDesc.offset = offset;
			}
		}

		if (createLayout)
		{
			VkDescriptorSetLayoutCreateInfo createInfo{
				.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.bindingCount	= (uint32_t)vkLayoutBindings.size(),
				.pBindings		= vkLayoutBindings.data()
			};

			VkDescriptorSetLayout vkDescriptorSetLayout{ VK_NULL_HANDLE };
			vkCreateDescriptorSetLayout(*m_device, &createInfo, nullptr, &vkDescriptorSetLayout);
			vkDescriptorSetLayouts.push_back(vkDescriptorSetLayout);

			wil::com_ptr<CVkDescriptorSetLayout> descriptorSetLayout = Create<CVkDescriptorSetLayout>(m_device.get(), vkDescriptorSetLayout);
			descriptorSetLayouts.push_back(descriptorSetLayout);

			layoutBindingMap[rootParamIndex] = layoutBindingDescs;
			rootParameterIndexToDescriptorSetMap[rootParamIndex] = (uint32_t)descriptorSetLayouts.size() - 1;
		}

		++rootParamIndex;
	}

	CVkPipelineLayout** pipelineLayoutRef = nullptr;
	bool firstCompile = false;
	{
		lock_guard<mutex> CS(m_pipelineLayoutMutex);

		auto iter = m_pipelineLayoutHashMap.find(hashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == m_pipelineLayoutHashMap.end())
		{
			pipelineLayoutRef = m_pipelineLayoutHashMap[hashCode].addressof();
			firstCompile = true;
		}
		else
		{
			pipelineLayoutRef = iter->second.addressof();
		}
	}

	wil::com_ptr<CVkPipelineLayout> pPipelineLayout;

	if (firstCompile)
	{
		VkPipelineLayoutCreateInfo createInfo{
			.sType						= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount				= (uint32_t)vkDescriptorSetLayouts.size(),
			.pSetLayouts				= vkDescriptorSetLayouts.data(),
			.pushConstantRangeCount		= (uint32_t)vkPushConstantRanges.size(),
			.pPushConstantRanges		= vkPushConstantRanges.data()
		};

		VkPipelineLayout vkPipelineLayout{ VK_NULL_HANDLE };
		vkCreatePipelineLayout(*m_device, &createInfo, nullptr, &vkPipelineLayout);

		pPipelineLayout = Create<CVkPipelineLayout>(m_device.get(), vkPipelineLayout);

		m_pipelineLayoutHashMap[hashCode] = pPipelineLayout;

		assert(*pipelineLayoutRef == pPipelineLayout);
	}
	else
	{
		while (*pipelineLayoutRef == nullptr)
		{
			this_thread::yield();
		}
		pPipelineLayout = *pipelineLayoutRef;
	}

	auto rootSignature = std::make_shared<RootSignature>();

	rootSignature->m_device = this;
	rootSignature->m_desc = rootSignatureDesc;
	rootSignature->m_pipelineLayout = pPipelineLayout;
	rootSignature->m_layoutBindingMap = layoutBindingMap;
	rootSignature->m_rootParameterIndexToDescriptorSetMap = rootParameterIndexToDescriptorSetMap;
	rootSignature->m_descriptorSetLayouts = descriptorSetLayouts;

	return rootSignature;
}


GraphicsPipelineStatePtr Device::CreateGraphicsPipelineState(const GraphicsPipelineDesc& pipelineDesc)
{
	// Shaders
	vector<VkPipelineShaderStageCreateInfo> shaderStages;

	auto AddShaderStageCreateInfo = [this, &shaderStages](ShaderType type, const ShaderNameAndEntry& shaderAndEntry)
		{
			if (shaderAndEntry)
			{
				Shader* shader = LoadShader(type, shaderAndEntry);
				assert(shader);
				auto shaderModule = CreateShaderModule(shader);

				VkPipelineShaderStageCreateInfo createInfo{};
				FillShaderStageCreateInfo(createInfo, *shaderModule, shader);
				shaderStages.push_back(createInfo);
			}
		};

	AddShaderStageCreateInfo(ShaderType::Vertex, pipelineDesc.vertexShader);
	AddShaderStageCreateInfo(ShaderType::Pixel, pipelineDesc.pixelShader);
	AddShaderStageCreateInfo(ShaderType::Geometry, pipelineDesc.geometryShader);
	AddShaderStageCreateInfo(ShaderType::Hull, pipelineDesc.hullShader);
	AddShaderStageCreateInfo(ShaderType::Domain, pipelineDesc.domainShader);

	// Vertex streams
	vector<VkVertexInputBindingDescription> vertexInputBindings;
	const auto& vertexStreams = pipelineDesc.vertexStreams;
	const uint32_t numStreams = (uint32_t)vertexStreams.size();
	if (numStreams > 0)
	{
		vertexInputBindings.resize(numStreams);
		for (uint32_t i = 0; i < numStreams; ++i)
		{
			vertexInputBindings[i].binding = vertexStreams[i].inputSlot;
			vertexInputBindings[i].inputRate = InputClassificationToVulkan(vertexStreams[i].inputClassification);
			vertexInputBindings[i].stride = vertexStreams[i].stride;
		}
	}

	// Vertex elements
	vector<VkVertexInputAttributeDescription> vertexAttributes;
	const auto& vertexElements = pipelineDesc.vertexElements;
	const uint32_t numElements = (uint32_t)vertexElements.size();
	if (numElements > 0)
	{
		vertexAttributes.resize(numElements);
		for (uint32_t i = 0; i < numElements; ++i)
		{
			vertexAttributes[i].binding = vertexElements[i].inputSlot;
			vertexAttributes[i].location = i;
			vertexAttributes[i].format = FormatToVulkan(vertexElements[i].format);
			vertexAttributes[i].offset = vertexElements[i].alignedByteOffset;
		}
	}

	// Vertex input layout
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{
		.sType								= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount		= (uint32_t)vertexInputBindings.size(),
		.pVertexBindingDescriptions			= vertexInputBindings.data(),
		.vertexAttributeDescriptionCount	= (uint32_t)vertexAttributes.size(),
		.pVertexAttributeDescriptions		= vertexAttributes.data()
	};

	// Input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
		.sType						= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology					= PrimitiveTopologyToVulkan(pipelineDesc.topology),
		.primitiveRestartEnable		= pipelineDesc.indexBufferStripCut == IndexBufferStripCutValue::Disabled ? VK_FALSE : VK_TRUE
	};

	// Tessellation state
	VkPipelineTessellationStateCreateInfo tessellationInfo{
		.sType					= VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
		.patchControlPoints		= GetControlPointCount(pipelineDesc.topology)
	};

	// Viewport state
	VkPipelineViewportStateCreateInfo viewportStateInfo{
		.sType			= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount	= 1,
		.pViewports		= nullptr, // dynamic state
		.scissorCount	= 1,
		.pScissors		= nullptr  // dynamic state
	};

	// Rasterizer state
	const auto rasterizerState = pipelineDesc.rasterizerState;
	VkPipelineRasterizationStateCreateInfo rasterizerInfo
	{
		.sType						= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable			= VK_FALSE,
		.rasterizerDiscardEnable	= VK_FALSE,
		.polygonMode				= FillModeToVulkan(rasterizerState.fillMode),
		.cullMode					= CullModeToVulkan(rasterizerState.cullMode),
		.frontFace					= rasterizerState.frontCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable			= (rasterizerState.depthBias != 0 || rasterizerState.slopeScaledDepthBias != 0.0f) ? VK_TRUE : VK_FALSE,
		.depthBiasConstantFactor	= *reinterpret_cast<const float*>(&rasterizerState.depthBias),
		.depthBiasClamp				= rasterizerState.depthBiasClamp,
		.depthBiasSlopeFactor		= rasterizerState.slopeScaledDepthBias,
		.lineWidth					= 1.0f
	};

	// Multisample state
	VkPipelineMultisampleStateCreateInfo multisampleInfo{
		.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples	= GetSampleCountFlags(pipelineDesc.msaaCount),
		.sampleShadingEnable	= VK_FALSE,
		.minSampleShading		= 0.0f,
		.pSampleMask			= nullptr,
		.alphaToCoverageEnable	= pipelineDesc.blendState.alphaToCoverageEnable ? VK_TRUE : VK_FALSE,
		.alphaToOneEnable		= VK_FALSE
	};

	// Depth stencil state
	const auto& depthStencilState = pipelineDesc.depthStencilState;
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo{
		.sType					= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable		= depthStencilState.depthEnable ? VK_TRUE : VK_FALSE,
		.depthWriteEnable		= depthStencilState.depthWriteMask == DepthWrite::All ? VK_TRUE : VK_FALSE,
		.depthCompareOp			= ComparisonFuncToVulkan(depthStencilState.depthFunc),
		.depthBoundsTestEnable	= VK_FALSE,
		.stencilTestEnable		= depthStencilState.stencilEnable ? VK_TRUE : VK_FALSE,
		.front = {
			.failOp			= StencilOpToVulkan(depthStencilState.frontFace.stencilFailOp),
			.passOp			= StencilOpToVulkan(depthStencilState.frontFace.stencilPassOp),
			.depthFailOp	= StencilOpToVulkan(depthStencilState.frontFace.stencilDepthFailOp),
			.compareOp		= ComparisonFuncToVulkan(depthStencilState.frontFace.stencilFunc),
			.compareMask	= depthStencilState.stencilReadMask,
			.writeMask		= depthStencilState.stencilWriteMask,
			.reference		= 0
		},
		.back = {
			.failOp			= StencilOpToVulkan(depthStencilState.backFace.stencilFailOp),
			.passOp			= StencilOpToVulkan(depthStencilState.backFace.stencilPassOp),
			.depthFailOp	= StencilOpToVulkan(depthStencilState.backFace.stencilDepthFailOp),
			.compareOp		= ComparisonFuncToVulkan(depthStencilState.backFace.stencilFunc),
			.compareMask	= depthStencilState.stencilReadMask,
			.writeMask		= depthStencilState.stencilWriteMask,
			.reference		= 0
		},
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f
	};

	// Blend state
	const auto& blendState = pipelineDesc.blendState;
	VkPipelineColorBlendStateCreateInfo blendStateInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };

	array<VkPipelineColorBlendAttachmentState, 8> blendAttachments;

	for (uint32_t i = 0; i < 8; ++i)
	{
		const auto& rt = blendState.renderTargetBlend[i];
		blendAttachments[i].blendEnable = rt.blendEnable ? VK_TRUE : VK_FALSE;
		blendAttachments[i].srcColorBlendFactor = BlendToVulkan(rt.srcBlend);
		blendAttachments[i].dstColorBlendFactor = BlendToVulkan(rt.dstBlend);
		blendAttachments[i].colorBlendOp = BlendOpToVulkan(rt.blendOp);
		blendAttachments[i].srcAlphaBlendFactor = BlendToVulkan(rt.srcBlendAlpha);
		blendAttachments[i].dstAlphaBlendFactor = BlendToVulkan(rt.dstBlendAlpha);
		blendAttachments[i].alphaBlendOp = BlendOpToVulkan(rt.blendOpAlpha);
		blendAttachments[i].colorWriteMask = ColorWriteToVulkan(rt.writeMask);

		// First render target with logic op enabled gets to set the state
		if (rt.logicOpEnable && (VK_FALSE == blendStateInfo.logicOpEnable))
		{
			blendStateInfo.logicOpEnable = VK_TRUE;
			blendStateInfo.logicOp = LogicOpToVulkan(rt.logicOp);
		}
	}
	const uint32_t numRtvs = (uint32_t)pipelineDesc.rtvFormats.size();
	blendStateInfo.attachmentCount = numRtvs;
	blendStateInfo.pAttachments = blendAttachments.data();

	// Dynamic states
	vector<VkDynamicState> dynamicStates;
	dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
	dynamicStates.push_back(VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY);

	VkPipelineDynamicStateCreateInfo dynamicStateInfo{
		.sType				= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount	= (uint32_t)dynamicStates.size(),
		.pDynamicStates		= dynamicStates.data()
	};

	// TODO: Check for dynamic rendering support here.  Will need proper extension/feature system.
	vector<VkFormat> rtvFormats(numRtvs);
	if (numRtvs > 0)
	{
		for (uint32_t i = 0; i < numRtvs; ++i)
		{
			rtvFormats[i] = FormatToVulkan(pipelineDesc.rtvFormats[i]);
		}
	}

	const Format dsvFormat = pipelineDesc.dsvFormat;
	VkPipelineRenderingCreateInfo dynamicRenderingInfo{
		.sType						= VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
		.viewMask					= 0,
		.colorAttachmentCount		= (uint32_t)rtvFormats.size(),
		.pColorAttachmentFormats	= rtvFormats.data(),
		.depthAttachmentFormat		= FormatToVulkan(dsvFormat),
		.stencilAttachmentFormat	= IsStencilFormat(dsvFormat) ? FormatToVulkan(dsvFormat) : VK_FORMAT_UNDEFINED
	};

	auto rootSignature = (RootSignature*)pipelineDesc.rootSignature.get();

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{
		.sType					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext					= &dynamicRenderingInfo,
		.stageCount				= (uint32_t)shaderStages.size(),
		.pStages				= shaderStages.data(),
		.pVertexInputState		= &vertexInputInfo,
		.pInputAssemblyState	= &inputAssemblyInfo,
		.pTessellationState		= &tessellationInfo,
		.pViewportState			= &viewportStateInfo,
		.pRasterizationState	= &rasterizerInfo,
		.pMultisampleState		= &multisampleInfo,
		.pDepthStencilState		= &depthStencilInfo,
		.pColorBlendState		= &blendStateInfo,
		.pDynamicState			= &dynamicStateInfo,
		.layout					= rootSignature->GetPipelineLayout(),
		.renderPass				= VK_NULL_HANDLE,
		.subpass				= 0,
		.basePipelineHandle		= VK_NULL_HANDLE,
		.basePipelineIndex		= 0
	};

	VkPipeline vkPipeline{ VK_NULL_HANDLE };
	wil::com_ptr<CVkPipeline> pipeline;
	if (VK_SUCCEEDED(vkCreateGraphicsPipelines(*m_device, *m_pipelineCache, 1, &pipelineCreateInfo, nullptr, &vkPipeline)))
	{
		pipeline = Create<CVkPipeline>(m_device.get(), vkPipeline);
	}
	else
	{
		LogError(LogVulkan) << "Failed to create VkPipeline (graphics).  Error code: " << res << endl;
	}

	auto pipelineState = std::make_shared<GraphicsPipelineState>();

	pipelineState->m_device = this;
	pipelineState->m_pipelineState = pipeline;

	return pipelineState;
}


DescriptorSetPtr Device::CreateDescriptorSet(const DescriptorSetDesc& descriptorSetDesc)
{
	std::lock_guard guard(m_descriptorSetMutex);

	// Find or create descriptor set pool
	VkDescriptorSetLayout vkDescriptorSetLayout = descriptorSetDesc.descriptorSetLayout->Get();
	DescriptorPool* pool{ nullptr };
	auto it = m_setPoolMapping.find(vkDescriptorSetLayout);
	if (it == m_setPoolMapping.end())
	{
		DescriptorPoolDesc descriptorPoolDesc{
			.device						= m_device.get(),
			.layout						= descriptorSetDesc.descriptorSetLayout,
			.rootParameter				= descriptorSetDesc.rootParameter,
			.poolSize					= MaxSetsPerPool,
			.allowFreeDescriptorSets	= true
		};

		auto poolHandle = make_unique<DescriptorPool>(descriptorPoolDesc);
		pool = poolHandle.get();
		m_setPoolMapping.emplace(vkDescriptorSetLayout, std::move(poolHandle));
	}
	else
	{
		pool = it->second.get();
	}

	// Allocate descriptor set from pool
	VkDescriptorSet vkDescriptorSet = pool->AllocateDescriptorSet();

	auto descriptorSet = std::make_shared<DescriptorSet>();

	descriptorSet->m_device = this;
	descriptorSet->m_descriptorSet = vkDescriptorSet;
	descriptorSet->m_numDescriptors = descriptorSetDesc.numDescriptors;
	descriptorSet->m_isDynamicBuffer = descriptorSetDesc.isDynamicBuffer;

	return descriptorSet;
}


SamplerPtr Device::CreateSampler(const SamplerDesc& samplerDesc)
{
	VkTextureFilterMapping filterMapping = TextureFilterToVulkan(samplerDesc.filter);

	VkSamplerCreateInfo createInfo{
		.sType						= VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.pNext						= nullptr,
		.flags						= 0,
		.magFilter					= filterMapping.magFilter,
		.minFilter					= filterMapping.minFilter,
		.mipmapMode					= filterMapping.mipFilter,
		.addressModeU				= TextureAddressToVulkan(samplerDesc.addressU),
		.addressModeV				= TextureAddressToVulkan(samplerDesc.addressV),
		.addressModeW				= TextureAddressToVulkan(samplerDesc.addressW),
		.mipLodBias					= samplerDesc.mipLODBias,
		.anisotropyEnable			= filterMapping.isAnisotropic,
		.maxAnisotropy				= (float)samplerDesc.maxAnisotropy,
		.compareEnable				= filterMapping.isComparisonEnabled,
		.compareOp					= ComparisonFuncToVulkan(samplerDesc.comparisonFunc),
		.minLod						= samplerDesc.minLOD,
		.maxLod						= samplerDesc.maxLOD,
		.borderColor				= VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK, // TODO
		.unnormalizedCoordinates	= VK_FALSE
	};

	wil::com_ptr<CVkSampler> pSampler;

	size_t hashValue = Utility::HashState(&createInfo);

	std::lock_guard lock(m_samplerMutex);

	auto iter = m_samplerMap.find(hashValue);
	if (iter != m_samplerMap.end())
	{
		pSampler = iter->second;
	}
	else
	{
		VkSampler vkSampler = VK_NULL_HANDLE;
		vkCreateSampler(m_device->Get(), &createInfo, nullptr, &vkSampler);

		pSampler = Create<CVkSampler>(m_device.get(), vkSampler);
	}

	VkDescriptorImageInfo imageInfoSampler{
		.sampler		= pSampler->Get(),
		.imageView		= VK_NULL_HANDLE,
		.imageLayout	= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	};

	auto samplerPtr = std::make_shared<Sampler>();

	samplerPtr->m_device = this;
	samplerPtr->m_sampler = pSampler;
	samplerPtr->m_imageInfoSampler = imageInfoSampler;

	return samplerPtr;
};


TexturePtr Device::CreateTexture1D(const TextureDesc& textureDesc)
{
	return CreateTextureSimple(TextureDimension::Texture1D, textureDesc);
}


TexturePtr Device::CreateTexture2D(const TextureDesc& textureDesc)
{
	return CreateTextureSimple(TextureDimension::Texture2D, textureDesc);
}


TexturePtr Device::CreateTexture3D(const TextureDesc& textureDesc)
{
	return CreateTextureSimple(TextureDimension::Texture3D, textureDesc);
}


ITexture* Device::CreateUninitializedTexture(const std::string& name, const std::string& mapKey)
{
	Texture* tex = new Texture();
	tex->m_name = name;
	tex->m_mapKey = mapKey;
	return tex;
}


bool Device::InitializeTexture(ITexture* texture, const TextureInitializer& texInit)
{
	Texture* textureVK = (Texture*)texture;
	assert(textureVK != nullptr);

	textureVK->m_device = this;
	textureVK->m_type = TextureDimensionToResourceType(texInit.dimension);
	textureVK->m_usageState = ResourceState::Undefined;
	textureVK->m_width = texInit.width;
	textureVK->m_height = texInit.height;
	textureVK->m_arraySizeOrDepth = texInit.arraySizeOrDepth;
	textureVK->m_numMips = texInit.numMips;
	textureVK->m_numSamples = 1;
	textureVK->m_planeCount = 1;
	textureVK->m_format = texInit.format;

	// Create image
	ImageDesc imageDesc{
		.name				= textureVK->m_name,
		.width				= texInit.width,
		.height				= texInit.height,
		.arraySizeOrDepth	= texInit.arraySizeOrDepth,
		.format				= texInit.format,
		.numMips			= texInit.numMips,
		.numSamples			= 1,
		.resourceType		= textureVK->m_type,
		.imageUsage			= GpuImageUsage::ShaderResource | GpuImageUsage::CopyDest,
		.memoryAccess		= MemoryAccess::GpuReadWrite
	};

	textureVK->m_image = CreateImage(imageDesc);

	// Shader resource view
	ImageViewDesc imageViewDesc{
		.image				= textureVK->m_image.get(),
		.name				= textureVK->m_name,
		.resourceType		= textureVK->GetResourceType(),
		.imageUsage			= GpuImageUsage::ShaderResource,
		.format				= textureVK->GetFormat(),
		.imageAspect		= ImageAspect::Color,
		.baseMipLevel		= 0,
		.mipCount			= textureVK->GetNumMips(),
		.baseArraySlice		= 0,
		.arraySize			= textureVK->GetArraySize()
	};
	textureVK->m_imageViewSrv = CreateImageView(imageViewDesc);

	VkDescriptorImageInfo imageInfoSrv{
		.sampler = VK_NULL_HANDLE,
		.imageView = textureVK->m_imageViewSrv->Get(),
		.imageLayout = GetImageLayout(ResourceState::ShaderResource)
	};
	textureVK->m_imageInfoSrv = imageInfoSrv;

	// Copy initial data
	CommandContext::InitializeTexture(texture, texInit);

	return true;
}


ColorBufferPtr Device::CreateColorBufferFromSwapChainImage(CVkImage* swapChainImage, uint32_t width, uint32_t height, Format format, uint32_t imageIndex)
{
	const std::string name = std::format("Primary Swapchain Image {}", imageIndex);

	// Swapchain image
	ColorBufferDesc colorBufferDesc{
		.name				= name,
		.resourceType		= ResourceType::Texture2D,
		.width				= width,
		.height				= height,
		.arraySizeOrDepth	= 1,
		.numSamples			= 1,
		.format				= format
	};

	SetDebugName(*m_device, swapChainImage->Get(), name);

	// RTV view
	ImageViewDesc imageViewDesc{
		.image				= swapChainImage,
		.name				= std::format("Primary Swapchain {} RTV Image View", imageIndex),
		.resourceType		= ResourceType::Texture2D,
		.imageUsage			= GpuImageUsage::RenderTarget,
		.format				= format,
		.imageAspect		= ImageAspect::Color,
		.baseMipLevel		= 0,
		.mipCount			= 1,
		.baseArraySlice		= 0,
		.arraySize			= 1
	};

	auto imageViewRtv = CreateImageView(imageViewDesc);

	// SRV view
	imageViewDesc
		.SetImageUsage(GpuImageUsage::ShaderResource)
		.SetName(std::format("Primary SwapChain {} SRV Image View", imageIndex));

	auto imageViewSrv = CreateImageView(imageViewDesc);

	// Descriptors
	VkDescriptorImageInfo imageInfoSrv{ VK_NULL_HANDLE, *imageViewSrv, GetImageLayout(ResourceState::ShaderResource) };
	VkDescriptorImageInfo imageInfoUav{ VK_NULL_HANDLE, *imageViewSrv, GetImageLayout(ResourceState::UnorderedAccess) };

	auto colorBuffer = std::make_shared<Luna::VK::ColorBuffer>();

	colorBuffer->m_device = this;
	colorBuffer->m_type = colorBufferDesc.resourceType;
	colorBuffer->m_usageState = ResourceState::Undefined;
	colorBuffer->m_width = colorBufferDesc.width;
	colorBuffer->m_height = colorBufferDesc.height;
	colorBuffer->m_arraySizeOrDepth = colorBufferDesc.arraySizeOrDepth;
	colorBuffer->m_numMips = colorBufferDesc.numMips;
	colorBuffer->m_numSamples = colorBufferDesc.numSamples;
	colorBuffer->m_format = colorBufferDesc.format;
	colorBuffer->m_dimension = ResourceTypeToTextureDimension(colorBuffer->m_type);
	colorBuffer->m_clearColor = colorBufferDesc.clearColor;
	colorBuffer->m_image = swapChainImage;
	colorBuffer->m_imageViewRtv = imageViewRtv;
	colorBuffer->m_imageViewSrv = imageViewSrv;
	colorBuffer->m_imageInfoSrv = imageInfoSrv;
	colorBuffer->m_imageInfoUav = imageInfoUav;

	return colorBuffer;
}


wil::com_ptr<CVkImage> Device::CreateImage(const ImageDesc& imageDesc)
{
	const bool isArray = HasAnyFlag(imageDesc.resourceType, ResourceType::TextureArray_Type);

	VkImageCreateInfo imageCreateInfo{
		.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.flags			= (VkImageCreateFlags)GetImageCreateFlags(imageDesc.resourceType),
		.imageType		= GetImageType(imageDesc.resourceType),
		.format			= FormatToVulkan(imageDesc.format),
		.extent			= {
			.width = (uint32_t)imageDesc.width,
			.height = imageDesc.height,
			.depth = isArray ? 1 : imageDesc.arraySizeOrDepth },
		.mipLevels = imageDesc.numMips,
		.arrayLayers	= isArray ? imageDesc.arraySizeOrDepth : 1,
		.samples		= GetSampleCountFlags(imageDesc.numSamples),
		.tiling			= VK_IMAGE_TILING_OPTIMAL,
		.usage			= GetImageUsageFlags(imageDesc.imageUsage)
	};

	// Remove storage flag if this format doesn't support it.
	// TODO - Make a table with all the format properties?
	VkFormatProperties properties = GetFormatProperties(m_device->GetPhysicalDevice(), imageDesc.format);
	if (!QueryOptimalTilingFeature(properties, VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT))
	{
		imageCreateInfo.usage &= ~VK_IMAGE_USAGE_STORAGE_BIT;
	}

	VmaAllocationCreateInfo imageAllocCreateInfo{};
	imageAllocCreateInfo.flags = GetMemoryFlags(imageDesc.memoryAccess);
	imageAllocCreateInfo.usage = GetMemoryUsage(imageDesc.memoryAccess);

	VkImage vkImage{ VK_NULL_HANDLE };
	VmaAllocation vmaAllocation{ VK_NULL_HANDLE };
	if (VK_SUCCEEDED(vmaCreateImage(*m_allocator, &imageCreateInfo, &imageAllocCreateInfo, &vkImage, &vmaAllocation, nullptr)))
	{
		SetDebugName(*m_device, vkImage, imageDesc.name);

		return Create<CVkImage>(m_device.get(), m_allocator.get(), vkImage, vmaAllocation);
	}
	else
	{
		LogError(LogVulkan) << "Failed to create VkImage.  Error code: " << res << std::endl;
	}

	return nullptr;
}


wil::com_ptr<CVkImageView> Device::CreateImageView(const ImageViewDesc& imageViewDesc)
{
	VkImageViewCreateInfo createInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	createInfo.viewType = GetImageViewType(imageViewDesc.resourceType, imageViewDesc.imageUsage);
	createInfo.format = FormatToVulkan(imageViewDesc.format);
	if (IsColorFormat(imageViewDesc.format))
	{
		createInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_A;
	}
	createInfo.subresourceRange = {};
	createInfo.subresourceRange.aspectMask = GetImageAspect(imageViewDesc.imageAspect);
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = imageViewDesc.mipCount;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = imageViewDesc.resourceType == ResourceType::Texture3D ? 1 : imageViewDesc.arraySize;
	createInfo.image = imageViewDesc.image->Get();

	VkImageView vkImageView{ VK_NULL_HANDLE };
	if (VK_SUCCEEDED(vkCreateImageView(*m_device, &createInfo, nullptr, &vkImageView)))
	{
		return Create<CVkImageView>(m_device.get(), imageViewDesc.image, vkImageView);
	}
	else
	{
		LogWarning(LogVulkan) << "Failed to create VkImageView.  Error code: " << res << std::endl;
	}

	return nullptr;
}


wil::com_ptr<CVkShaderModule> Device::CreateShaderModule(Shader* shader)
{
	CVkShaderModule** ppShaderModule = nullptr;
	bool firstCompile = false;
	{
		lock_guard<mutex> CS(m_shaderModuleMutex);

		size_t hashCode = shader->GetHash();
		auto iter = m_shaderModuleHashMap.find(hashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == m_shaderModuleHashMap.end())
		{
			ppShaderModule = m_shaderModuleHashMap[hashCode].addressof();
			firstCompile = true;
		}
		else
		{
			ppShaderModule = &iter->second;
		}
	}

	if (firstCompile)
	{
		VkShaderModuleCreateInfo createInfo{
			.sType		= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize	= shader->GetByteCodeSize(),
			.pCode		= reinterpret_cast<const uint32_t*>(shader->GetByteCode())
		};

		VkShaderModule vkShaderModule{ VK_NULL_HANDLE };
		vkCreateShaderModule(*m_device, &createInfo, nullptr, &vkShaderModule);

		wil::com_ptr<CVkShaderModule> shaderModule = Create<CVkShaderModule>(m_device.get(), vkShaderModule);

		*ppShaderModule = shaderModule.get();

		(*ppShaderModule)->AddRef();
	}

	return *ppShaderModule;
}


wil::com_ptr<CVkPipelineCache> Device::CreatePipelineCache() const
{
	VkPipelineCacheCreateInfo createInfo{
		.sType				= VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
		.initialDataSize	= 0,
		.pInitialData		= nullptr
	};

	VkPipelineCache vkPipelineCache{ VK_NULL_HANDLE };
	if (VK_SUCCEEDED(vkCreatePipelineCache(*m_device, &createInfo, nullptr, &vkPipelineCache)))
	{
		return Create<CVkPipelineCache>(m_device.get(), vkPipelineCache);
	}
	else
	{
		LogError(LogVulkan) << "Failed to create VkPipelineCache.  Error code: " << res << endl;
	}

	return nullptr;
}


TexturePtr Device::CreateTextureSimple(TextureDimension dimension, const TextureDesc& textureDesc)
{
	const size_t height = dimension == TextureDimension::Texture1D ? 1 : textureDesc.height;
	const size_t depth = dimension == TextureDimension::Texture3D ? textureDesc.depth : 1;

	size_t numBytes = 0;
	size_t rowPitch = 0;
	GetSurfaceInfo(textureDesc.width, height, textureDesc.format, &numBytes, &rowPitch, nullptr, nullptr, nullptr);

	TextureInitializer texInit{
		.format				= textureDesc.format,
		.dimension			= dimension,
		.width				= textureDesc.width,
		.height				= (uint32_t)height,
		.arraySizeOrDepth	= (uint32_t)depth,
		.numMips			= 1
	};
	texInit.subResourceData.push_back(TextureSubresourceData{});

	size_t skipMip = 0;
	FillTextureInitializer(
		textureDesc.width,
		height,
		depth,
		1, // numMips
		1, // arraySize
		textureDesc.format,
		0, // maxSize
		numBytes,
		textureDesc.data,
		skipMip,
		texInit);

	TexturePtr texture = CreateUninitializedTexture(textureDesc.name, textureDesc.name);
	InitializeTexture(texture.Get(), texInit);

	return texture;
}


Device* GetVulkanDevice()
{
	assert(g_vulkanDevice != nullptr);

	return g_vulkanDevice;
}

} // namespace Luna::VK