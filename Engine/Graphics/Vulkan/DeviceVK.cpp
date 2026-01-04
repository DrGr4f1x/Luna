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
#include "DescriptorAllocatorVK.h"
#include "DescriptorSetVK.h"
#include "GpuBufferVK.h"
#include "PipelineStateVK.h"
#include "PipelineStateUtilVK.h"
#include "QueryHeapVK.h"
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
	case RootParameterType::RootCBV: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	case RootParameterType::RootSRV:
	case RootParameterType::RootUAV: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
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


Device::Device(CVkDevice* device, CVmaAllocator* allocator, const Luna::DeviceCaps& caps)
	: m_device{ device }
	, m_allocator{ allocator }
	, m_caps{ caps }
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

	// Create ColorBuffer
	auto colorBuffer = std::make_shared<Luna::VK::ColorBuffer>();
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
	colorBuffer->m_rtvDescriptor.SetImageView(image.get(), imageViewRtv.get());
	colorBuffer->m_srvDescriptor.SetImageView(image.get(), imageViewSrv.get());
	colorBuffer->m_uavDescriptor.SetImageView(image.get(), imageViewSrv.get());
	colorBuffer->m_srvDescriptor.ReadRawDescriptor(this, DescriptorType::TextureSRV);
	colorBuffer->m_uavDescriptor.ReadRawDescriptor(this, DescriptorType::TextureUAV);
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

	// Create DepthBuffer
	auto depthBuffer = std::make_shared<Luna::VK::DepthBuffer>();
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
	depthBuffer->m_depthStencilDescriptor.SetImageView(image.get(), imageViewDepthStencil.get());
	depthBuffer->m_depthOnlyDescriptor.SetImageView(image.get(), imageViewDepthOnly.get());
	depthBuffer->m_stencilOnlyDescriptor.SetImageView(image.get(), imageViewStencilOnly.get());
	depthBuffer->m_depthStencilDescriptor.ReadRawDescriptor(this, DescriptorType::TextureSRV);
	depthBuffer->m_depthOnlyDescriptor.ReadRawDescriptor(this, DescriptorType::TextureSRV);
	depthBuffer->m_stencilOnlyDescriptor.ReadRawDescriptor(this, DescriptorType::TextureSRV);

	return depthBuffer;
}


GpuBufferPtr Device::CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc)
{
	constexpr VkBufferUsageFlags transferFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

#if USE_DESCRIPTOR_BUFFERS
	VkBufferUsageFlags extraFlags = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
#endif // USE_DESCRIPTOR_BUFFERS

#if USE_LEGACY_DESCRIPTOR_SETS
	VkBufferUsageFlags extraFlags{};
#endif // USE_LEGACY_DESCRIPTOR_SETS

	if (gpuBufferDesc.bAllowShaderResource || gpuBufferDesc.bAllowUnorderedAccess)
		extraFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	if (gpuBufferDesc.resourceType == ResourceType::IndirectArgsBuffer)
		extraFlags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

	const bool isTypedBuffer = gpuBufferDesc.resourceType == ResourceType::TypedBuffer;

	VkBufferCreateInfo bufferCreateInfo{
		.sType	= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size	= gpuBufferDesc.elementCount * gpuBufferDesc.elementSize,
		.usage	= GetBufferUsageFlags(gpuBufferDesc.resourceType) | transferFlags | extraFlags
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
	if (isTypedBuffer)
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

	// Create GpuBuffer
	auto gpuBuffer = std::make_shared<GpuBuffer>();

	gpuBuffer->m_type = gpuBufferDesc.resourceType;
	gpuBuffer->m_format = gpuBufferDesc.format;
	gpuBuffer->m_usageState = ResourceState::GenericRead;
	gpuBuffer->m_elementSize = gpuBufferDesc.elementSize;
	gpuBuffer->m_elementCount = gpuBufferDesc.elementCount;
	gpuBuffer->m_bufferSize = gpuBuffer->m_elementCount * gpuBuffer->m_elementSize;
	gpuBuffer->m_buffer = buffer;
	gpuBuffer->m_isCpuWriteable = HasFlag(gpuBufferDesc.memoryAccess, MemoryAccess::CpuWrite);

	// Descriptor setup
	
	if (gpuBufferDesc.bAllowShaderResource)
	{
		gpuBuffer->m_srvDescriptor.SetBufferView(buffer.get(), bufferView.get(), gpuBufferDesc.elementSize, FormatToVulkan(gpuBufferDesc.format));

		if (HasAnyFlag(gpuBufferDesc.resourceType, ResourceType::TypedBuffer))
		{
			gpuBuffer->m_srvDescriptor.ReadRawDescriptor(this, DescriptorType::TypedBufferSRV);
		}
		else if (HasAnyFlag(gpuBufferDesc.resourceType, ResourceType::ByteAddressBuffer))
		{
			gpuBuffer->m_srvDescriptor.ReadRawDescriptor(this, DescriptorType::RawBufferSRV);
		}
		else
		{
			gpuBuffer->m_srvDescriptor.ReadRawDescriptor(this, DescriptorType::StructuredBufferSRV);
		}
	}
	if (gpuBufferDesc.bAllowUnorderedAccess)
	{
		gpuBuffer->m_uavDescriptor.SetBufferView(buffer.get(), bufferView.get(), gpuBufferDesc.elementSize, FormatToVulkan(gpuBufferDesc.format));

		if (HasAnyFlag(gpuBufferDesc.resourceType, ResourceType::TypedBuffer))
		{
			gpuBuffer->m_uavDescriptor.ReadRawDescriptor(this, DescriptorType::TypedBufferUAV);
		}
		else if (HasAnyFlag(gpuBufferDesc.resourceType, ResourceType::ByteAddressBuffer))
		{
			gpuBuffer->m_uavDescriptor.ReadRawDescriptor(this, DescriptorType::RawBufferUAV);
		}
		else
		{
			gpuBuffer->m_uavDescriptor.ReadRawDescriptor(this, DescriptorType::StructuredBufferUAV);
		}
	}
	if (HasAnyFlag(gpuBufferDesc.resourceType, ResourceType::ConstantBuffer))
	{
		gpuBuffer->m_cbvDescriptor.SetBufferView(buffer.get(), bufferView.get(), gpuBufferDesc.elementSize, FormatToVulkan(gpuBufferDesc.format));
		gpuBuffer->m_cbvDescriptor.ReadRawDescriptor(this, DescriptorType::ConstantBuffer);
	}
	
	if (gpuBufferDesc.initialData)
	{
		if (gpuBuffer->m_type == ResourceType::ConstantBuffer)
		{
			const size_t initialSize = gpuBuffer->GetBufferSize();
			gpuBuffer->Update(initialSize, gpuBufferDesc.initialData);
		}
		else
		{
			GpuBufferPtr temp = gpuBuffer;
			CommandContext::InitializeBuffer(temp, gpuBufferDesc.initialData, gpuBuffer->GetBufferSize());
		}
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

	vector<VkDescriptorSetLayout> vkDescriptorSetLayouts;
	vector<DescriptorSetLayoutPtr> descriptorSetLayouts;
	vector<VkPushConstantRange> vkPushConstantRanges;
	uint32_t pushConstantOffset{ 0 };

	vkDescriptorSetLayouts.resize(rootSignatureDesc.rootParameters.size());
	descriptorSetLayouts.resize(rootSignatureDesc.rootParameters.size());

	// Push descriptors
	uint32_t pushDescriptorSetIndex = (uint32_t)-1;
	vector<pair<uint32_t, RootParameter>> pushDescriptorRootParameters;
	unordered_map<uint32_t, uint32_t> pushDescriptorBindingMap;

	size_t hashCode = Utility::g_hashStart;

	uint32_t rootParamIndex = 0;
	for (const auto& rootParameter : rootSignatureDesc.rootParameters)
	{
		VkShaderStageFlags shaderStageFlags = ShaderStageToVulkan(rootParameter.shaderVisibility);

		uint32_t offset = 0;

		DescriptorSetLayoutPtr descriptorSetLayout;

		// Push constants
		if (rootParameter.parameterType == RootParameterType::RootConstants)
		{
			VkPushConstantRange& vkPushConstantRange = vkPushConstantRanges.emplace_back();
			vkPushConstantRange.offset = pushConstantOffset;
			vkPushConstantRange.size = rootParameter.num32BitConstants * 4;
			vkPushConstantRange.stageFlags = shaderStageFlags;
			pushConstantOffset += rootParameter.num32BitConstants * 4;

			hashCode = Utility::HashState(&vkPushConstantRange, 1, hashCode);

			auto emptySetLayout = GetOrCreateEmptyDescriptorSetLayout();
			vkDescriptorSetLayouts[rootParamIndex] = emptySetLayout->GetDescriptorSetLayout()->Get();
			descriptorSetLayouts[rootParamIndex] = emptySetLayout;
		}
		// Push descriptors
		else if (IsRootDescriptorType(rootParameter.parameterType))
		{
			if (pushDescriptorSetIndex == (uint32_t)-1)
			{
				pushDescriptorSetIndex = rootParamIndex;
			}

			pushDescriptorRootParameters.push_back(make_pair(rootParamIndex, rootParameter));

			pushDescriptorBindingMap[rootParamIndex] = rootParameter.startRegister;

			auto emptySetLayout = GetOrCreateEmptyDescriptorSetLayout();
			vkDescriptorSetLayouts[rootParamIndex] = emptySetLayout->GetDescriptorSetLayout()->Get();
			descriptorSetLayouts[rootParamIndex] = emptySetLayout;
		}
		// Regular descriptor sets
		else if (rootParameter.parameterType == RootParameterType::Table)
		{
			descriptorSetLayout = CreateDescriptorSetLayout(rootParameter);

			hashCode = Utility::HashMerge(hashCode, descriptorSetLayout->GetHashCode());

			vkDescriptorSetLayouts[rootParamIndex] = descriptorSetLayout->GetDescriptorSetLayout()->Get();
			descriptorSetLayouts[rootParamIndex] = descriptorSetLayout;
		}

		++rootParamIndex;
	}

	// Setup static samplers
	vector<SamplerPtr> samplers;
#if USE_LEGACY_DESCRIPTOR_SETS
	VkDescriptorSet staticSamplerDescriptorSet = VK_NULL_HANDLE;
#endif // USE_LEGACY_DESCRIPTOR_SETS
	uint32_t staticSamplerDescriptorSetIndex = ~0u;
	if (!rootSignatureDesc.staticSamplers.empty())
	{
		vector<VkSampler> vkSamplers;
		vector<VkDescriptorSetLayoutBinding> vkLayoutBindings;

		// First, loop over the StaticSamplerDescs to create the Samplers and store off the 
		// internal VkSamplers, which we need to populate the VkDescriptorSetLayoutBindings
		// in the second pass
		for (const auto& staticSamplerDesc : rootSignatureDesc.staticSamplers)
		{
			auto sampler = CreateSampler(staticSamplerDesc.samplerDesc);
			samplers.push_back(sampler);

			Sampler* samplerVK = (Sampler*)sampler.get();

			vkSamplers.push_back(samplerVK->GetSampler());
		}

		// Now, loop over the StaticSamplerDescs again to create the VkDescriptorSetLayoutBindings
		uint32_t currentBinding = 0;
		for (const auto& staticSamplerDesc : rootSignatureDesc.staticSamplers)
		{
			VkDescriptorSetLayoutBinding vkBinding{
				.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_ALL,
				.pImmutableSamplers = &vkSamplers[currentBinding]
			};

			if (staticSamplerDesc.shaderRegister == APPEND_REGISTER)
			{
				vkBinding.binding = currentBinding++;
			}
			else
			{
				vkBinding.binding = staticSamplerDesc.shaderRegister;
				currentBinding = vkBinding.binding + 1;
			}

			vkBinding.binding += GetRegisterShift(DescriptorType::Sampler);

			vkLayoutBindings.push_back(vkBinding);

			hashCode = Utility::HashState(&vkBinding, 1, hashCode);
		}

		// Create the descriptor set layout for the static samplers
		VkDescriptorSetLayoutCreateInfo createInfo{
			.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
#if USE_DESCRIPTOR_BUFFERS
			.flags			= VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT | VK_DESCRIPTOR_SET_LAYOUT_CREATE_EMBEDDED_IMMUTABLE_SAMPLERS_BIT_EXT,
#endif // USE_DESCRIPTOR_BUFFERS
			.bindingCount	= (uint32_t)vkLayoutBindings.size(),
			.pBindings		= vkLayoutBindings.data()
		};

		VkDescriptorSetLayout vkDescriptorSetLayout{ VK_NULL_HANDLE };
		vkCreateDescriptorSetLayout(*m_device, &createInfo, nullptr, &vkDescriptorSetLayout);
		vkDescriptorSetLayouts.push_back(vkDescriptorSetLayout);

		staticSamplerDescriptorSetIndex = (uint32_t)descriptorSetLayouts.size();

		auto descriptorSetLayout = make_shared<DescriptorSetLayout>();
		descriptorSetLayout->m_descriptorSetLayout = Create<CVkDescriptorSetLayout>(m_device.get(), vkDescriptorSetLayout);
		descriptorSetLayouts.push_back(descriptorSetLayout);

#if USE_LEGACY_DESCRIPTOR_SETS
		staticSamplerDescriptorSet = AllocateDescriptorSet(vkDescriptorSetLayout);
#endif // USE_LEGACY_DESCRIPTOR_SETS
	}
	
	// Setup push descriptor set
	if (!pushDescriptorRootParameters.empty())
	{
		assert(pushDescriptorSetIndex != (uint32_t)-1);

		vector<VkDescriptorSetLayoutBinding> vkLayoutBindings;

		for (const auto& rootParameterPair : pushDescriptorRootParameters)
		{
			uint32_t rootIndex = rootParameterPair.first;
			const auto& rootParameter = rootParameterPair.second;

			const uint32_t regShift = GetRegisterShift(rootParameter.parameterType);

			VkDescriptorSetLayoutBinding vkBinding{
				.binding				= regShift + rootParameter.startRegister,
				.descriptorType			= RootParameterTypeToVulkanDescriptorType(rootParameter.parameterType),
				.descriptorCount		= 1,
				.stageFlags				= ShaderStageToVulkan(rootParameter.shaderVisibility),
				.pImmutableSamplers		= nullptr
			};
			vkLayoutBindings.push_back(vkBinding);

			hashCode = Utility::HashState(&vkBinding, 1, hashCode);
		}

		// Create the descriptor set layout for the push descriptors
		VkDescriptorSetLayoutCreateInfo createInfo{
			.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
#if USE_DESCRIPTOR_BUFFERS
			.flags			= VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT | VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
#endif // USE_DESCRIPTOR_BUFFERs
#if USE_LEGACY_DESCRIPTOR_SETS
			.flags			= VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT,
#endif // USE_LEGACY_DESCRIPTOR_SETS
			.bindingCount	= (uint32_t)vkLayoutBindings.size(),
			.pBindings		= vkLayoutBindings.data()
		};

		VkDescriptorSetLayout vkDescriptorSetLayout{ VK_NULL_HANDLE };
		vkCreateDescriptorSetLayout(*m_device, &createInfo, nullptr, &vkDescriptorSetLayout);

		vkDescriptorSetLayouts[pushDescriptorSetIndex] = vkDescriptorSetLayout;

		auto descriptorSetLayout = make_shared<DescriptorSetLayout>();
		descriptorSetLayout->m_descriptorSetLayout = Create<CVkDescriptorSetLayout>(m_device.get(), vkDescriptorSetLayout);
		descriptorSetLayouts[pushDescriptorSetIndex] = descriptorSetLayout;
	}

	// Finally, create the VkPipelineLayout
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
	rootSignature->m_descriptorSetLayouts = descriptorSetLayouts;
	rootSignature->m_staticSamplers = samplers;
	rootSignature->m_staticSamplerDescriptorSetIndex = staticSamplerDescriptorSetIndex;
#if USE_LEGACY_DESCRIPTOR_SETS
	rootSignature->m_staticSamplerDescriptorSet = staticSamplerDescriptorSet;
#endif // USE_LEGACY_DESCRIPTOR_SETS
	rootSignature->m_pushDescriptorSetIndex = pushDescriptorSetIndex;
	rootSignature->m_pushDescriptorBindingMap = pushDescriptorBindingMap;

	return rootSignature;
}


GraphicsPipelinePtr Device::CreateGraphicsPipeline(const GraphicsPipelineDesc& pipelineDesc)
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
		uint32_t curOffset = 0;
		for (uint32_t i = 0; i < numElements; ++i)
		{
			vertexAttributes[i].binding = vertexElements[i].inputSlot;
			vertexAttributes[i].location = i;
			vertexAttributes[i].format = FormatToVulkan(vertexElements[i].format);
			if (vertexElements[i].alignedByteOffset == APPEND_ALIGNED_ELEMENT)
			{
				vertexAttributes[i].offset = curOffset;
			}
			else
			{
				vertexAttributes[i].offset = vertexElements[i].alignedByteOffset;
			}
			curOffset += BlockSize(vertexElements[i].format);
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
	VkPipelineViewportStateCreateInfo viewportStateInfo{};
	FillViewportState(viewportStateInfo);

	// Rasterizer state
	VkPipelineRasterizationStateCreateInfo rasterizerInfo{};
	FillRasterizerState(rasterizerInfo, pipelineDesc.rasterizerState);

	// Multisample state
	VkPipelineMultisampleStateCreateInfo multisampleInfo{};
	FillMultisampleState(multisampleInfo, pipelineDesc);

	// Depth stencil state
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
	FillDepthStencilState(depthStencilInfo, pipelineDesc.depthStencilState);

	// Blend state
	VkPipelineColorBlendStateCreateInfo blendStateInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	array<VkPipelineColorBlendAttachmentState, 8> blendAttachments;
	FillBlendState(blendStateInfo, blendAttachments, pipelineDesc);

	// Dynamic states
	VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
	vector<VkDynamicState> dynamicStates;
	FillDynamicStates(dynamicStateInfo, dynamicStates, false /*isMeshletState*/);

	// TODO: Check for dynamic rendering support here.  Will need proper extension/feature system.
	const uint32_t numRtvs = (uint32_t)pipelineDesc.rtvFormats.size();
	vector<VkFormat> rtvFormats(numRtvs);
	VkPipelineRenderingCreateInfo dynamicRenderingInfo{};
	FillRenderTargetState(dynamicRenderingInfo, rtvFormats, pipelineDesc);

	auto rootSignature = (RootSignature*)pipelineDesc.rootSignature.get();

	// Flags
#if USE_DESCRIPTOR_BUFFERS
	VkPipelineCreateFlags flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
#endif // USE_DESCRIPTOR_BUFFERS

#if USE_LEGACY_DESCRIPTOR_SETS
	VkPipelineCreateFlags flags{};
#endif // USE_LEGACY_DESCRIPTOR_SETS
	
	VkGraphicsPipelineCreateInfo pipelineCreateInfo{
		.sType					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext					= &dynamicRenderingInfo,
		.flags					= flags,
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

	auto graphicsPipeline = std::make_shared<GraphicsPipeline>();

	graphicsPipeline->m_device = this;
	graphicsPipeline->m_desc = pipelineDesc;
	graphicsPipeline->m_pipelineState = pipeline;

	return graphicsPipeline;
}


ComputePipelinePtr Device::CreateComputePipeline(const ComputePipelineDesc& pipelineDesc)
{
	// Shaders
	VkPipelineShaderStageCreateInfo shaderStage{};

	Shader* shader = LoadShader(ShaderType::Compute, pipelineDesc.computeShader);
	assert(shader);
	auto shaderModule = CreateShaderModule(shader);

	FillShaderStageCreateInfo(shaderStage, *shaderModule, shader);

	auto rootSignature = (RootSignature*)pipelineDesc.rootSignature.get();

	// Flags
#if USE_DESCRIPTOR_BUFFERS
	VkPipelineCreateFlags flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
#endif // USE_DESCRIPTOR_BUFFERS

#if USE_LEGACY_DESCRIPTOR_SETS
	VkPipelineCreateFlags flags{};
#endif // USE_LEGACY_DESCRIPTOR_SETS

	VkComputePipelineCreateInfo pipelineCreateInfo{
		.sType					= VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext					= nullptr,
		.flags					= flags,
		.stage					= shaderStage,
		.layout					= rootSignature->GetPipelineLayout(),
		.basePipelineHandle		= VK_NULL_HANDLE,
		.basePipelineIndex		= 0
	};

	VkPipeline vkPipeline{ VK_NULL_HANDLE };
	wil::com_ptr<CVkPipeline> pipeline;
	if (VK_SUCCEEDED(vkCreateComputePipelines(*m_device, *m_pipelineCache, 1, &pipelineCreateInfo, nullptr, &vkPipeline)))
	{
		pipeline = Create<CVkPipeline>(m_device.get(), vkPipeline);
	}
	else
	{
		LogError(LogVulkan) << "Failed to create VkPipeline (compute).  Error code: " << res << endl;
	}

	auto computePipeline = std::make_shared<ComputePipeline>();

	computePipeline->m_device = this;
	computePipeline->m_desc = pipelineDesc;
	computePipeline->m_pipelineState = pipeline;

	return computePipeline;
}


MeshletPipelinePtr Device::CreateMeshletPipeline(const MeshletPipelineDesc& pipelineDesc)
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

	AddShaderStageCreateInfo(ShaderType::Amplification, pipelineDesc.amplificationShader);
	AddShaderStageCreateInfo(ShaderType::Mesh, pipelineDesc.meshShader);
	AddShaderStageCreateInfo(ShaderType::Pixel, pipelineDesc.pixelShader);

	// Viewport state
	VkPipelineViewportStateCreateInfo viewportStateInfo{};
	FillViewportState(viewportStateInfo);

	// Rasterizer state
	VkPipelineRasterizationStateCreateInfo rasterizerInfo{};
	FillRasterizerState(rasterizerInfo, pipelineDesc.rasterizerState);

	// Multisample state
	VkPipelineMultisampleStateCreateInfo multisampleInfo{};
	FillMultisampleState(multisampleInfo, pipelineDesc);

	// Depth stencil state
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
	FillDepthStencilState(depthStencilInfo, pipelineDesc.depthStencilState);

	// Blend state
	VkPipelineColorBlendStateCreateInfo blendStateInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	array<VkPipelineColorBlendAttachmentState, 8> blendAttachments;
	FillBlendState(blendStateInfo, blendAttachments, pipelineDesc);

	// Dynamic states
	VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
	vector<VkDynamicState> dynamicStates;
	FillDynamicStates(dynamicStateInfo, dynamicStates, true /*isMeshletState*/);

	// TODO: Check for dynamic rendering support here.  Will need proper extension/feature system.
	const uint32_t numRtvs = (uint32_t)pipelineDesc.rtvFormats.size();
	vector<VkFormat> rtvFormats(numRtvs);
	VkPipelineRenderingCreateInfo dynamicRenderingInfo{};
	FillRenderTargetState(dynamicRenderingInfo, rtvFormats, pipelineDesc);

	auto rootSignature = (RootSignature*)pipelineDesc.rootSignature.get();

	// Flags
#if USE_DESCRIPTOR_BUFFERS
	VkPipelineCreateFlags flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
#endif // USE_DESCRIPTOR_BUFFERS

#if USE_LEGACY_DESCRIPTOR_SETS
	VkPipelineCreateFlags flags{};
#endif // USE_LEGACY_DESCRIPTOR_SETS
	
	VkGraphicsPipelineCreateInfo pipelineCreateInfo{
		.sType					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext					= &dynamicRenderingInfo,
		.flags					= flags,
		.stageCount				= (uint32_t)shaderStages.size(),
		.pStages				= shaderStages.data(),
		.pVertexInputState		= nullptr,
		.pInputAssemblyState	= nullptr,
		.pTessellationState		= nullptr,
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
		LogError(LogVulkan) << "Failed to create VkPipeline (meshlet).  Error code: " << res << endl;
	}

	auto meshletPipeline = std::make_shared<MeshletPipeline>();

	meshletPipeline->m_device = this;
	meshletPipeline->m_desc = pipelineDesc;
	meshletPipeline->m_pipelineState = pipeline;

	return meshletPipeline;
}


QueryHeapPtr Device::CreateQueryHeap(const QueryHeapDesc& queryHeapDesc)
{
	VkQueryPipelineStatisticFlags pipelineStatisticFlags = 0;

	if (queryHeapDesc.type == QueryHeapType::PipelineStats)
	{
		pipelineStatisticFlags |= VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT;
		pipelineStatisticFlags |= VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT;
		pipelineStatisticFlags |= VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT;
		pipelineStatisticFlags |= VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT;
		pipelineStatisticFlags |= VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT;
		pipelineStatisticFlags |= VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT;
		pipelineStatisticFlags |= VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT;
		pipelineStatisticFlags |= VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT;
		pipelineStatisticFlags |= VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT;
		pipelineStatisticFlags |= VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT;
		pipelineStatisticFlags |= VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;
		// TODO: Support mesh shader stats
	}

	VkQueryPoolCreateInfo createInfo{
		.sType					= VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
		.queryType				= QueryHeapTypeToVulkan(queryHeapDesc.type),
		.queryCount				= queryHeapDesc.queryCount,
		.pipelineStatistics		= pipelineStatisticFlags
	};

	wil::com_ptr<CVkQueryPool> queryPool;

	VkQueryPool vkQueryPool = VK_NULL_HANDLE;
	if (VK_SUCCEEDED(vkCreateQueryPool(m_device->Get(), &createInfo, nullptr, &vkQueryPool)))
	{
		queryPool = Create<CVkQueryPool>(m_device.get(), vkQueryPool);
	}
	else
	{
		LogError(LogVulkan) << "Failed to create VkQueryPool.  Error code: " << res << endl;
	}

	auto queryHeap = make_shared<QueryHeap>();
	queryHeap->m_desc = queryHeapDesc;
	queryHeap->m_device = this;
	queryHeap->m_pool = queryPool;
	return queryHeap;
}


DescriptorSetPtr Device::CreateDescriptorSet(const DescriptorSetDesc& descriptorSetDesc)
{
	std::lock_guard guard(m_descriptorSetMutex);

#if USE_DESCRIPTOR_BUFFERS
	VkDescriptorSetLayout vkDescriptorSetLayout = descriptorSetDesc.descriptorSetLayout->GetDescriptorSetLayout()->Get();
	assert(vkDescriptorSetLayout != VK_NULL_HANDLE);

	// TODO: Handle sampler descriptor sets
	DescriptorBufferAllocation allocation = AllocateDescriptorBufferMemory(DescriptorBufferType::Resource, vkDescriptorSetLayout);

	auto descriptorSet = std::make_shared<DescriptorSet>(this, descriptorSetDesc.rootParameter);
	descriptorSet->m_allocation = allocation;
	descriptorSet->m_layout = descriptorSetDesc.descriptorSetLayout;

	return descriptorSet;
#endif // USE_DESCRIPTOR_BUFFERS

#if USE_LEGACY_DESCRIPTOR_SETS
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

	auto descriptorSet = std::make_shared<DescriptorSet>(this, descriptorSetDesc.rootParameter);
	descriptorSet->m_descriptorSet = vkDescriptorSet;
	descriptorSet->m_numDescriptors = descriptorSetDesc.numDescriptors;

	return descriptorSet;
#endif // USE_LEGACY_DESCRIPTOR_SETS
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
		.borderColor				= BorderColorToVulkan(samplerDesc.staticBorderColor),
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

	// Create Sampler
	auto samplerPtr = std::make_shared<Sampler>();
	samplerPtr->m_descriptor.SetSampler(pSampler.get());
	samplerPtr->m_descriptor.ReadRawDescriptor(this, DescriptorType::Sampler);

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

	const ResourceType type = TextureDimensionToResourceType(texInit.dimension);
	const bool isCubemap = HasAnyFlag(type, ResourceType::TextureCube_Type);
	const bool isVolume = HasFlag(type, ResourceType::Texture3D);
	uint32_t effectiveArraySize = texInit.arraySizeOrDepth;
	if (type == ResourceType::TextureCube)
	{
		effectiveArraySize = 1;
	}
	else if (type == ResourceType::TextureCube_Array)
	{
		effectiveArraySize /= 6;
	}
	
	textureVK->m_device = this;
	textureVK->m_type = type;
	textureVK->m_usageState = ResourceState::Undefined;
	textureVK->m_width = texInit.width;
	textureVK->m_height = texInit.height;
	textureVK->m_arraySizeOrDepth = effectiveArraySize;
	textureVK->m_numMips = texInit.numMips;
	textureVK->m_numSamples = 1;
	textureVK->m_planeCount = 1;
	textureVK->m_format = texInit.format;
	textureVK->m_dimension = texInit.dimension;

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
		.arraySize			= isVolume ? 1 : texInit.arraySizeOrDepth
	};
	auto imageView = CreateImageView(imageViewDesc);

	textureVK->m_descriptor.SetImageView(textureVK->m_image.get(), imageView.get());
	textureVK->m_descriptor.ReadRawDescriptor(this, DescriptorType::TextureSRV);

	// Copy initial data
	TexturePtr temp = texture;
	CommandContext::InitializeTexture(temp, texInit);

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
	VkDescriptorImageInfo imageInfoSrv{ VK_NULL_HANDLE, *imageViewSrv, GetImageLayout(ResourceState::PixelShaderResource) };
	VkDescriptorImageInfo imageInfoUav{ VK_NULL_HANDLE, *imageViewSrv, GetImageLayout(ResourceState::UnorderedAccess) };

	auto colorBuffer = std::make_shared<Luna::VK::ColorBuffer>();

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
	colorBuffer->m_rtvDescriptor.SetImageView(swapChainImage, imageViewRtv.get());
	colorBuffer->m_srvDescriptor.SetImageView(swapChainImage, imageViewSrv.get());
	colorBuffer->m_srvDescriptor.ReadRawDescriptor(this, DescriptorType::TextureSRV);

	return colorBuffer;
}


#if USE_DESCRIPTOR_BUFFERS
wil::com_ptr<CVkBuffer> Device::CreateDescriptorBuffer(DescriptorBufferType type, size_t sizeInBytes)
{
	VkBufferUsageFlags extraFlags = type == DescriptorBufferType::Resource ?
		VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT :
		VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT;
	extraFlags |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

	VkBufferCreateInfo bufferCreateInfo{
		.sType	= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size	= sizeInBytes,
		.usage	= extraFlags
	};

	VmaAllocationCreateInfo allocCreateInfo{};
	const MemoryAccess memoryAccess = MemoryAccess::CpuWrite | MemoryAccess::GpuRead;
	allocCreateInfo.flags = GetMemoryFlags(memoryAccess);
	allocCreateInfo.usage = GetMemoryUsage(memoryAccess);

	VkBuffer vkBuffer{ VK_NULL_HANDLE };
	VmaAllocation vmaBufferAllocation{ VK_NULL_HANDLE };

	auto res = vmaCreateBuffer(*m_allocator, &bufferCreateInfo, &allocCreateInfo, &vkBuffer, &vmaBufferAllocation, nullptr);
	assert(res == VK_SUCCESS);

	SetDebugName(*m_device, vkBuffer, "Descriptor Buffer");

	auto buffer = Create<CVkBuffer>(m_device.get(), m_allocator.get(), vkBuffer, vmaBufferAllocation);

	return buffer;
}
#endif // USE_DESCRIPTOR_BUFFERS


wil::com_ptr<CVkImage> Device::CreateImage(const ImageDesc& imageDesc)
{
	const bool isVolume = HasAnyFlag(imageDesc.resourceType, ResourceType::Texture3D);
	const bool isArray = HasAnyFlag(imageDesc.resourceType, ResourceType::TextureArray_Type);
	const bool isCubemap = HasAnyFlag(imageDesc.resourceType, ResourceType::TextureCube_Type);

	const uint32_t depth = isVolume ? imageDesc.arraySizeOrDepth : 1;
	const uint32_t arraySize = (isArray || isCubemap) ? imageDesc.arraySizeOrDepth : 1;

	VkImageCreateInfo imageCreateInfo{
		.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.flags			= (VkImageCreateFlags)GetImageCreateFlags(imageDesc.resourceType),
		.imageType		= GetImageType(imageDesc.resourceType),
		.format			= FormatToVulkan(imageDesc.format),
		.extent = {
			.width	= (uint32_t)imageDesc.width,
			.height = imageDesc.height,
			.depth	= depth },
		.mipLevels		= imageDesc.numMips,
		.arrayLayers	= arraySize,
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
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	}
	createInfo.subresourceRange = {};
	createInfo.subresourceRange.aspectMask = GetImageAspect(imageViewDesc.imageAspect);
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = imageViewDesc.mipCount;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = imageViewDesc.arraySize;
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
	CVkShaderModule** shaderModuleRef = nullptr;

	size_t hashCode = shader->GetHash();

	bool firstCompile = false;
	{
		lock_guard<mutex> CS(m_shaderModuleMutex);
		auto iter = m_shaderModuleHashMap.find(hashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == m_shaderModuleHashMap.end())
		{
			shaderModuleRef = m_shaderModuleHashMap[hashCode].addressof();
			firstCompile = true;
		}
		else
		{
			shaderModuleRef = iter->second.addressof();
			assert(shaderModuleRef != nullptr);
		}
	}

	wil::com_ptr<CVkShaderModule> pShaderModule;

	if (firstCompile)
	{
		VkShaderModuleCreateInfo createInfo{
			.sType		= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize	= shader->GetByteCodeSize(),
			.pCode		= reinterpret_cast<const uint32_t*>(shader->GetByteCode())
		};

		VkShaderModule vkShaderModule{ VK_NULL_HANDLE };
		vkCreateShaderModule(*m_device, &createInfo, nullptr, &vkShaderModule);

		pShaderModule = Create<CVkShaderModule>(m_device.get(), vkShaderModule);

		m_shaderModuleHashMap[hashCode] = pShaderModule.get();

		assert(*shaderModuleRef == pShaderModule);
	}
	else
	{
		while (*shaderModuleRef == nullptr)
		{
			this_thread::yield();
		}
		pShaderModule = *shaderModuleRef;
	}

	return pShaderModule;
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


DescriptorSetLayoutPtr Device::CreateDescriptorSetLayout(const RootParameter& rootParameter)
{
	assert(rootParameter.parameterType == RootParameterType::Table);

	// Count descriptor bindings
	uint32_t numBindings = 0;
	for (const auto& range : rootParameter.table)
	{
		const bool isArray = HasAnyFlag(range.flags, DescriptorRangeFlags::Array | DescriptorRangeFlags::VariableSizedArray);
		numBindings += isArray ? 1 : range.numDescriptors;
	}

	vector<VkDescriptorSetLayoutBinding> bindings;
	vector<VkDescriptorBindingFlags> bindingFlags;
	bindings.reserve(numBindings);
	bindingFlags.reserve(numBindings);
	bool allowUpdateAfterSet = false;

	size_t hashCode = Utility::g_hashStart;

	uint32_t currentBinding[] = { 0, 0, 0, 0 };

	for (const auto& range : rootParameter.table)
	{
		const uint32_t baseBindingIndex = range.startRegister;

		VkDescriptorBindingFlags flags = 0;
		if (HasFlag(range.flags, DescriptorRangeFlags::PartiallyBound))
		{
			flags |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
		}
#if USE_LEGACY_DESCRIPTOR_SETS
		if (HasFlag(range.flags, DescriptorRangeFlags::AllowUpdateAfterSet))
		{
			flags |= VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
			allowUpdateAfterSet = true;
		}
#endif // USE_LEGACY_DESCRIPTOR_SETS

		uint32_t numDescriptors = 1;
		const bool isArray = HasAnyFlag(range.flags, DescriptorRangeFlags::Array | DescriptorRangeFlags::VariableSizedArray);

		if (isArray)
		{
#if USE_LEGACY_DESCRIPTOR_SETS
			if (HasFlag(range.flags, DescriptorRangeFlags::VariableSizedArray))
			{
				flags |= VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
			}
#endif // USE_LEGACY_DESCRIPTOR_SETS
		}
		else
		{
			numDescriptors = range.numDescriptors;
		}

		uint32_t currentBindingIndex = GetRegisterClass(range.descriptorType);

		for (uint32_t j = 0; j < numDescriptors; ++j)
		{
			VkDescriptorBindingFlags& bindFlags = bindingFlags.emplace_back();
			bindFlags = flags;

			VkDescriptorSetLayoutBinding& descriptorBinding = bindings.emplace_back();
			descriptorBinding = {};
			descriptorBinding.descriptorType = DescriptorTypeToVulkan(range.descriptorType);
			descriptorBinding.stageFlags = ShaderStageToVulkan(rootParameter.shaderVisibility);
			descriptorBinding.descriptorCount = isArray ? range.numDescriptors : 1;

			if (range.startRegister == APPEND_REGISTER)
			{
				descriptorBinding.binding = (currentBinding[currentBindingIndex]++) + j;
			}
			else
			{
				descriptorBinding.binding = range.startRegister + j;
				currentBinding[currentBindingIndex] = descriptorBinding.binding + 1;
			}

			descriptorBinding.binding += GetRegisterShift(range.descriptorType);

			hashCode = Utility::HashState(&bindFlags, 1, hashCode);
			hashCode = Utility::HashState(&descriptorBinding, 1, hashCode);
		}
	}

	VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{
		.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
		.bindingCount	= (uint32_t)bindingFlags.size(),
		.pBindingFlags	= bindingFlags.data()
	};

#if USE_DESCRIPTOR_BUFFERS
	VkDescriptorSetLayoutCreateFlags layoutFlags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
#endif // USE_DESCRIPTOR_BUFFFERS

#if USE_LEGACY_DESCRIPTOR_SETS
	VkDescriptorSetLayoutCreateFlags layoutFlags{};
#endif // USE_LEGACY_DESCRIPTOR_SETS

	if (allowUpdateAfterSet)
	{
		layoutFlags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
	}

	VkDescriptorSetLayoutCreateInfo info{ 
		.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext			= &bindingFlagsInfo, // TODO: Check device for descriptorIndexing support
		.flags			= layoutFlags,
		.bindingCount	= (uint32_t)bindings.size(),
		.pBindings		= bindings.data()
	};
	
	hashCode = Utility::HashState(&info, 1, hashCode);

	VkDescriptorSetLayout vkSetLayout = VK_NULL_HANDLE;
	vkCreateDescriptorSetLayout(*m_device, &info, nullptr, &vkSetLayout);

	wil::com_ptr<CVkDescriptorSetLayout> setLayout = Create<CVkDescriptorSetLayout>(m_device.get(), vkSetLayout);

	auto descriptorSetLayout = std::make_shared<DescriptorSetLayout>();

	descriptorSetLayout->m_device = this;
	descriptorSetLayout->m_descriptorSetLayout = setLayout;
	descriptorSetLayout->m_hashcode = hashCode;

#if USE_DESCRIPTOR_BUFFERS
	// Get info for descriptor buffer binding
	{
		VkDeviceSize layoutSize{ 0 };
		vkGetDescriptorSetLayoutSizeEXT(*m_device, vkSetLayout, &layoutSize);
		
		// TODO: Cache these
		auto bindingTemplate = make_shared<DescriptorBindingTemplate>();

		for (const auto& binding : bindings)
		{
			VkDeviceSize offset{ 0 };
			vkGetDescriptorSetLayoutBindingOffsetEXT(*m_device, vkSetLayout, binding.binding, &offset);
			
			bindingTemplate->m_bindingInfoMap[binding.binding] = { offset, binding.descriptorType };
		}

		descriptorSetLayout->m_layoutSize = layoutSize;
		descriptorSetLayout->m_bindingTemplate = bindingTemplate;
	}
#endif // USE_DESCRIPTOR_BUFFERS

	return descriptorSetLayout;
}


DescriptorSetLayoutPtr Device::GetOrCreateEmptyDescriptorSetLayout()
{
	lock_guard lock(m_emptyDescriptorSetLayoutMutex);

	if (!m_emptyDescriptorSetLayout)
	{
		VkDescriptorSetLayoutCreateInfo info{
			.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext			= nullptr,
#if USE_DESCRIPTOR_BUFFERS
			.flags			= VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
#endif // USE_DESCRIPTOR_BUFFERS
			.bindingCount	= 0,
			.pBindings		= nullptr
		};

		size_t hashCode = Utility::g_hashStart;
		hashCode = Utility::HashState(&info, 1, hashCode);

		VkDescriptorSetLayout vkSetLayout = VK_NULL_HANDLE;
		vkCreateDescriptorSetLayout(*m_device, &info, nullptr, &vkSetLayout);

		wil::com_ptr<CVkDescriptorSetLayout> setLayout = Create<CVkDescriptorSetLayout>(m_device.get(), vkSetLayout);

		m_emptyDescriptorSetLayout = std::make_shared<DescriptorSetLayout>();

		m_emptyDescriptorSetLayout->m_device = this;
		m_emptyDescriptorSetLayout->m_descriptorSetLayout = setLayout;
		m_emptyDescriptorSetLayout->m_hashcode = hashCode;
	}

	return m_emptyDescriptorSetLayout;
}


TexturePtr Device::CreateTextureSimple(TextureDimension dimension, const TextureDesc& textureDesc)
{
	const size_t height = dimension == TextureDimension::Texture1D ? 1 : textureDesc.height;
	const size_t depth = dimension == TextureDimension::Texture3D ? textureDesc.depth : 1;

	assert(textureDesc.dataSize != 0);
	assert(textureDesc.data != nullptr);

	TextureInitializer texInit{
		.format				= textureDesc.format,
		.dimension			= dimension,
		.width				= textureDesc.width,
		.height				= (uint32_t)height,
		.arraySizeOrDepth	= (uint32_t)depth,
		.numMips			= textureDesc.numMips
	};
	texInit.subResourceData.push_back(TextureSubresourceData{});

	size_t skipMip = 0;
	FillTextureInitializer(
		textureDesc.width,
		height,
		depth,
		textureDesc.numMips,
		1, // arraySize
		textureDesc.format,
		0, // maxSize
		textureDesc.dataSize,
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