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

#include "Graphics\Shader.h"

#include "ColorBufferVK.h"
#include "DepthBufferVK.h"
#include "GpuBufferVK.h"
#include "PipelineStateVK.h"
#include "RootSignatureVK.h"

using namespace std;

extern Luna::IGraphicsDevice* g_graphicsDevice;


namespace Luna::VK
{

GraphicsDevice* g_vulkanGraphicsDevice = nullptr;


// TODO - Move this elsewhere?
bool QueryLinearTilingFeature(VkFormatProperties properties, VkFormatFeatureFlagBits flags)
{
	return (properties.linearTilingFeatures & flags) != 0;
}


bool QueryOptimalTilingFeature(VkFormatProperties properties, VkFormatFeatureFlagBits flags)
{
	return (properties.optimalTilingFeatures & flags) != 0;
}


bool QueryBufferFeature(VkFormatProperties properties, VkFormatFeatureFlagBits flags)
{
	return (properties.bufferFeatures & flags) != 0;
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


GraphicsDevice::GraphicsDevice(const GraphicsDeviceDesc& desc)
	: m_desc{ desc }
	, m_vkDevice{ desc.device }
{
	g_graphicsDevice = this;
	g_vulkanGraphicsDevice = this;
}


GraphicsDevice::~GraphicsDevice()
{
	LogInfo(LogVulkan) << "Destroying Vulkan device." << endl;

	g_graphicsDevice = nullptr;
	g_vulkanGraphicsDevice = nullptr;
}


wil::com_ptr<IPlatformData> GraphicsDevice::CreateColorBufferData(ColorBufferDesc& desc, ResourceState& initialState)
{
	// Create image
	auto imageDesc = ImageDesc{
		.name				= desc.name,
		.width				= desc.width,
		.height				= desc.height,
		.arraySizeOrDepth	= desc.arraySizeOrDepth,
		.format				= desc.format,
		.numMips			= desc.numMips,
		.numSamples			= desc.numSamples,
		.resourceType		= desc.resourceType,
		.imageUsage			= GpuImageUsage::ColorBuffer,
		.memoryAccess		= MemoryAccess::GpuReadWrite
	};

	if (HasFlag(desc.resourceType, ResourceType::Texture3D))
	{
		imageDesc.SetNumMips(1);
		imageDesc.SetDepth(desc.arraySizeOrDepth);
	}
	else if (HasAnyFlag(desc.resourceType, ResourceType::Texture2D_Type))
	{
		if (HasAnyFlag(desc.resourceType, ResourceType::TextureArray_Type))
		{
			imageDesc.SetResourceType(desc.numSamples == 1 ? ResourceType::Texture2D_Array : ResourceType::Texture2DMS_Array);
			imageDesc.SetArraySize(desc.arraySizeOrDepth);
		}
		else
		{
			imageDesc.SetResourceType(desc.numSamples == 1 ? ResourceType::Texture2D : ResourceType::Texture2DMS_Array);
		}
	}

	auto image = CreateImage(imageDesc);

	// Render target view
	auto imageViewDesc = ImageViewDesc{
		.image				= image.get(),
		.name				= desc.name,
		.resourceType		= ResourceType::Texture2D,
		.imageUsage			= GpuImageUsage::RenderTarget,
		.format				= desc.format,
		.imageAspect		= ImageAspect::Color,
		.baseMipLevel		= 0,
		.mipCount			= desc.numMips,
		.baseArraySlice		= 0,
		.arraySize			= desc.arraySizeOrDepth
	};
	auto imageViewRtv = CreateImageView(imageViewDesc);

	// Shader resource view
	imageViewDesc.SetImageUsage(GpuImageUsage::ShaderResource);
	auto imageViewSrv = CreateImageView(imageViewDesc);

	// Descriptors
	auto imageInfoSrv = VkDescriptorImageInfo{
		.sampler		= VK_NULL_HANDLE,
		.imageView		= *imageViewSrv,
		.imageLayout	= GetImageLayout(ResourceState::ShaderResource)
	};
	auto imageInfoUav = VkDescriptorImageInfo{
		.sampler		= VK_NULL_HANDLE,
		.imageView		= *imageViewSrv,
		.imageLayout	= GetImageLayout(ResourceState::UnorderedAccess)
	};

	auto descExt = ColorBufferDescExt{
		.image			= image.get(),
		.imageViewRtv	= imageViewRtv.get(),
		.imageViewSrv	= imageViewSrv.get(),
		.imageInfoSrv	= imageInfoSrv,
		.imageInfoUav	= imageInfoUav,
		.usageState		= ResourceState::Common
	};

	initialState = ResourceState::Common;

	return Make<ColorBufferData>(descExt);
}


wil::com_ptr<IPlatformData> GraphicsDevice::CreateDepthBufferData(DepthBufferDesc& desc, ResourceState& initialState)
{
	// Create depth image
	auto imageDesc = ImageDesc{
		.name				= desc.name,
		.width				= desc.width,
		.height				= desc.height,
		.arraySizeOrDepth	= desc.arraySizeOrDepth,
		.format				= desc.format,
		.numMips			= desc.numMips,
		.numSamples			= desc.numSamples,
		.resourceType		= desc.resourceType,
		.imageUsage			= GpuImageUsage::DepthBuffer,
		.memoryAccess		= MemoryAccess::GpuReadWrite
	};
	
	auto image = CreateImage(imageDesc);

	// Create image views and descriptors
	const bool bHasStencil = IsStencilFormat(desc.format);

	auto imageAspect = ImageAspect::Depth;
	if (bHasStencil)
	{
		imageAspect |= ImageAspect::Stencil;
	}

	auto imageViewDesc = ImageViewDesc{
		.image				= image.get(),
		.name				= desc.name,
		.resourceType		= ResourceType::Texture2D,
		.imageUsage			= GpuImageUsage::DepthStencilTarget,
		.format				= desc.format,
		.imageAspect		= ImageAspect::Color,
		.baseMipLevel		= 0,
		.mipCount			= desc.numMips,
		.baseArraySlice		= 0,
		.arraySize			= desc.arraySizeOrDepth
	};

	auto imageViewDepthStencil = CreateImageView(imageViewDesc);
	wil::com_ptr<CVkImageView> imageViewDepthOnly;
	wil::com_ptr<CVkImageView> imageViewStencilOnly;

	if (bHasStencil)
	{
		imageViewDesc
			.SetName(format("{} Depth Image View", desc.name))
			.SetImageAspect(ImageAspect::Depth)
			.SetViewType(TextureSubresourceViewType::DepthOnly);

		imageViewDepthOnly = CreateImageView(imageViewDesc);

		imageViewDesc
			.SetName(format("{} Stencil Image View", desc.name))
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

	auto descExt = DepthBufferDescExt{
		.image					= image.get(),
		.imageViewDepthStencil	= imageViewDepthStencil.get(),
		.imageViewDepthOnly		= imageViewDepthOnly.get(),
		.imageViewStencilOnly	= imageViewStencilOnly.get(),
		.imageInfoDepth			= imageInfoDepth,
		.imageInfoStencil		= imageInfoStencil,
		.usageState				= ResourceState::DepthRead | ResourceState::DepthWrite
	};

	return Make<DepthBufferData>(descExt);
}


wil::com_ptr<IPlatformData> GraphicsDevice::CreateGpuBufferData(GpuBufferDesc& desc, ResourceState& initialState)
{
	constexpr VkBufferUsageFlags transferFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	auto createInfo = VkBufferCreateInfo{
		.sType	= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext	= nullptr,
		.size	= desc.elementCount * desc.elementSize,
		.usage	= GetBufferUsageFlags(desc.resourceType) | transferFlags
	};

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.flags = GetMemoryFlags(desc.memoryAccess);
	allocCreateInfo.usage = GetMemoryUsage(desc.memoryAccess);

	VkBuffer vkBuffer{ VK_NULL_HANDLE };
	VmaAllocation vmaBufferAllocation{ VK_NULL_HANDLE };

	auto res = vmaCreateBuffer(*m_vmaAllocator, &createInfo, &allocCreateInfo, &vkBuffer, &vmaBufferAllocation, nullptr);
	assert(res == VK_SUCCESS);

	SetDebugName(*m_vkDevice, vkBuffer, desc.name);

	auto buffer = Create<CVkBuffer>(m_vkDevice.get(), m_vmaAllocator.get(), vkBuffer, vmaBufferAllocation);

	auto descExt = GpuBufferDescExt{
		.buffer			= buffer.get(),
		.bufferInfo		= {
			.buffer		= vkBuffer,
			.offset		= 0,
			.range		= VK_WHOLE_SIZE
		}
	};
	return Make<GpuBufferData>(descExt);
}


wil::com_ptr<IPlatformData> GraphicsDevice::CreateGraphicsPSOData(GraphicsPSODesc& desc)
{
	vector<VkDynamicState> dynamicStates{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	// Blend state
	VkPipelineColorBlendStateCreateInfo blendStateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0
	};
	
	array<VkPipelineColorBlendAttachmentState, 8> blendAttachments;

	for (uint32_t i = 0; i < 8; ++i)
	{
		const auto& rt = desc.blendState.renderTargetBlend[i];
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
			blendStateInfo.logicOp = static_cast<VkLogicOp>(rt.logicOp);
		}
	}
	blendStateInfo.attachmentCount = (uint32_t)desc.rtvFormats.size();
	blendStateInfo.pAttachments = blendAttachments.data();

	// Multisample state
	VkPipelineMultisampleStateCreateInfo multisampleInfo{ 
		.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.pNext					= nullptr,
		.flags					= 0,
		.rasterizationSamples	= GetSampleCountFlags(desc.msaaCount),
		.sampleShadingEnable	= VK_FALSE,
		.minSampleShading		= 0.0f,
		.pSampleMask			= nullptr,
		.alphaToCoverageEnable	= desc.blendState.alphaToCoverageEnable ? VK_TRUE : VK_FALSE,
		.alphaToOneEnable		= VK_FALSE,
	};
	
	// Rasterizer state
	VkPipelineRasterizationStateCreateInfo rasterizerInfo{ 
		.sType						= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.pNext						= nullptr,
		.flags						= 0,
		.depthClampEnable			= VK_FALSE,
		.rasterizerDiscardEnable	= VK_FALSE,
		.polygonMode				= FillModeToVulkan(desc.rasterizerState.fillMode),
		.cullMode					= CullModeToVulkan(desc.rasterizerState.cullMode),
		.frontFace					= desc.rasterizerState.frontCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable			= (desc.rasterizerState.depthBias != 0 || desc.rasterizerState.slopeScaledDepthBias != 0.0f) ? VK_TRUE : VK_FALSE,
		.depthBiasConstantFactor	= *reinterpret_cast<const float*>(&desc.rasterizerState.depthBias),
		.depthBiasSlopeFactor		= desc.rasterizerState.slopeScaledDepthBias,
		.lineWidth					= 1.0f
	};
	
	// Depth-stencil state
	// TODO: Use dynamic state for: stencil read/write masks and stencil ref
	// TODO: Figure out how to handle depth bounds test (for DX12)
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo{
		.sType					= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.pNext					= nullptr,
		.flags					= 0,
		.depthTestEnable		= desc.depthStencilState.depthEnable ? VK_TRUE : VK_FALSE,
		.depthWriteEnable		= (desc.depthStencilState.depthWriteMask == DepthWrite::All) ? VK_TRUE : VK_FALSE,
		.depthCompareOp			= ComparisonFuncToVulkan(desc.depthStencilState.depthFunc),
		.depthBoundsTestEnable	= VK_FALSE,
		.stencilTestEnable		= desc.depthStencilState.stencilEnable ? VK_TRUE : VK_FALSE,
		.front = {
			.failOp			= StencilOpToVulkan(desc.depthStencilState.frontFace.stencilFailOp),
			.passOp			= StencilOpToVulkan(desc.depthStencilState.frontFace.stencilPassOp),
			.depthFailOp	= StencilOpToVulkan(desc.depthStencilState.frontFace.stencilDepthFailOp),
			.compareOp		= ComparisonFuncToVulkan(desc.depthStencilState.frontFace.stencilFunc),
			.compareMask	= desc.depthStencilState.stencilReadMask,
			.writeMask		= desc.depthStencilState.stencilWriteMask,
			.reference		= 0
		},
		.back = {
			.failOp			= StencilOpToVulkan(desc.depthStencilState.backFace.stencilFailOp),
			.passOp			= StencilOpToVulkan(desc.depthStencilState.backFace.stencilPassOp),
			.depthFailOp	= StencilOpToVulkan(desc.depthStencilState.backFace.stencilDepthFailOp),
			.compareOp		= ComparisonFuncToVulkan(desc.depthStencilState.backFace.stencilFunc),
			.compareMask	= desc.depthStencilState.stencilReadMask,
			.writeMask		= desc.depthStencilState.stencilWriteMask,
			.reference		= 0
		},
		.minDepthBounds			= 0.0f,
		.maxDepthBounds			= 1.0f
	};
	
	// Primitive topology & primitive restart
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{ 
		.sType						= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext						= nullptr,
		.flags						= 0,
		.topology					= PrimitiveTopologyToVulkan(desc.topology),
		.primitiveRestartEnable		= (desc.indexBufferStripCut == IndexBufferStripCutValue::Disabled) ? VK_FALSE : VK_TRUE
	};
	
	VkPipelineTessellationStateCreateInfo tessellationInfo { 
		.sType					= VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
		.pNext					= nullptr,
		.flags					= 0,
		.patchControlPoints		= GetControlPointCount(desc.topology)
	};
	
	// Input layout
	const uint32_t numStreams = (uint32_t)desc.vertexStreams.size();
	vector<VkVertexInputBindingDescription> vertexInputBindings(numStreams);
	for (uint32_t i = 0; i < numStreams; ++i)
	{
		vertexInputBindings[i].binding = desc.vertexStreams[i].inputSlot;
		vertexInputBindings[i].inputRate = InputClassificationToVulkan(desc.vertexStreams[i].inputClassification);
		vertexInputBindings[i].stride = desc.vertexStreams[i].stride;
	}

	const uint32_t numAttributes = (uint32_t)desc.vertexElements.size();
	vector<VkVertexInputAttributeDescription> vertexAttributes(numAttributes);
	for (uint32_t i = 0; i < numAttributes; ++i)
	{
		vertexAttributes[i].binding = desc.vertexElements[i].inputSlot;
		vertexAttributes[i].location = i;
		vertexAttributes[i].format = FormatToVulkan(desc.vertexElements[i].format);
		vertexAttributes[i].offset = desc.vertexElements[i].alignedByteOffset;
	}

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{ 
		.sType								= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext								= nullptr,
		.flags								= 0,
		.vertexBindingDescriptionCount		= (uint32_t)vertexInputBindings.size(),
		.pVertexBindingDescriptions			= vertexInputBindings.data(),
		.vertexAttributeDescriptionCount	= (uint32_t)vertexAttributes.size(),
		.pVertexAttributeDescriptions		= vertexAttributes.data()
	};
	
	// TODO: Shaders
	

	return nullptr;
}


ColorBufferHandle GraphicsDevice::CreateColorBuffer(const ColorBufferDesc& colorBufferDesc)
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

	ColorBufferDescExt colorBufferDescExt{
		.image			= image.get(),
		.imageViewRtv	= imageViewRtv.get(),
		.imageViewSrv	= imageViewSrv.get(),
		.imageInfoSrv	= imageInfoSrv,
		.imageInfoUav	= imageInfoUav,
		.usageState		= ResourceState::Common
	};

	return Make<ColorBufferVK>(colorBufferDesc, colorBufferDescExt);
}


DepthBufferHandle GraphicsDevice::CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc)
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
		.imageAspect		= ImageAspect::Color,
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

	DepthBufferDescExt depthBufferDescExt{
		.image					= image.get(),
		.imageViewDepthStencil	= imageViewDepthStencil.get(),
		.imageViewDepthOnly		= imageViewDepthOnly.get(),
		.imageViewStencilOnly	= imageViewStencilOnly.get(),
		.imageInfoDepth			= imageInfoDepth,
		.imageInfoStencil		= imageInfoStencil,
		.usageState				= ResourceState::DepthRead | ResourceState::DepthWrite
	};

	return Make<DepthBufferVK>(depthBufferDesc, depthBufferDescExt);
}


GpuBufferHandle GraphicsDevice::CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc)
{
	constexpr VkBufferUsageFlags transferFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VkBufferCreateInfo bufferCreateInfo{
		.sType	= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext	= nullptr,
		.size	= gpuBufferDesc.elementCount * gpuBufferDesc.elementSize,
		.usage	= GetBufferUsageFlags(gpuBufferDesc.resourceType) | transferFlags
	};

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.flags = GetMemoryFlags(gpuBufferDesc.memoryAccess);
	allocCreateInfo.usage = GetMemoryUsage(gpuBufferDesc.memoryAccess);

	VkBuffer vkBuffer{ VK_NULL_HANDLE };
	VmaAllocation vmaBufferAllocation{ VK_NULL_HANDLE };

	auto res = vmaCreateBuffer(*m_vmaAllocator, &bufferCreateInfo, &allocCreateInfo, &vkBuffer, &vmaBufferAllocation, nullptr);
	assert(res == VK_SUCCESS);

	SetDebugName(*m_vkDevice, vkBuffer, gpuBufferDesc.name);

	auto buffer = Create<CVkBuffer>(m_vkDevice.get(), m_vmaAllocator.get(), vkBuffer, vmaBufferAllocation);

	GpuBufferDescExt gpuBufferDescExt{
		.buffer = buffer.get(),
		.bufferInfo = {
			.buffer = vkBuffer,
			.offset = 0,
			.range = VK_WHOLE_SIZE
		}
	};
	return Make<GpuBufferVK>(gpuBufferDesc, gpuBufferDescExt);
}


RootSignatureHandle GraphicsDevice::CreateRootSignature(const RootSignatureDesc& rootSignatureDesc)
{
	std::vector<VkDescriptorSetLayout> vkDescriptorSetLayouts;
	std::vector<wil::com_ptr<CVkDescriptorSetLayout>> descriptorSetLayouts;
	std::vector<VkPushConstantRange> vkPushConstantRanges;
	uint32_t pushConstantOffset{ 0 };

	size_t hashCode = Utility::g_hashStart;

	// Build merged list of root parameters
	std::vector<RootParameter> rootParameters;
	for (const auto& rootParameterSet : rootSignatureDesc.rootParameters)
	{
		rootParameters.insert(rootParameters.end(), rootParameterSet.begin(), rootParameterSet.end());
	}

	for (const auto& rootParameter : rootParameters)
	{
		VkShaderStageFlags shaderStageFlags = ShaderStageToVulkan(rootParameter.shaderVisibility);

		if (rootParameter.parameterType == RootParameterType::RootConstants)
		{
			VkPushConstantRange& vkPushConstantRange = vkPushConstantRanges.emplace_back();
			vkPushConstantRange.offset = pushConstantOffset;
			vkPushConstantRange.size = rootParameter.num32BitConstants * 4;
			vkPushConstantRange.stageFlags = shaderStageFlags;
			pushConstantOffset += rootParameter.num32BitConstants * 4;

			hashCode = Utility::HashState(&vkPushConstantRange, 1, hashCode);
		}
		else if (rootParameter.parameterType == RootParameterType::RootCBV)
		{
			VkDescriptorSetLayoutBinding vkBinding{
				.binding				= rootParameter.startRegister + rootSignatureDesc.bindingOffsets.constantBuffer,
				.descriptorType			= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
				.descriptorCount		= 1,
				.stageFlags				= shaderStageFlags,
				.pImmutableSamplers		= nullptr
			};

			hashCode = Utility::HashState(&vkBinding, 1, hashCode);

			VkDescriptorSetLayoutCreateInfo createInfo{
				.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.pNext			= nullptr,
				.flags			= 0,
				.bindingCount	= 1,
				.pBindings		= &vkBinding
			};

			VkDescriptorSetLayout vkDescriptorSetLayout{ VK_NULL_HANDLE };
			vkCreateDescriptorSetLayout(*m_vkDevice, &createInfo, nullptr, &vkDescriptorSetLayout);
			vkDescriptorSetLayouts.push_back(vkDescriptorSetLayout);

			wil::com_ptr<CVkDescriptorSetLayout> descriptorSetLayout = Create<CVkDescriptorSetLayout>(m_vkDevice.get(), vkDescriptorSetLayout);
			descriptorSetLayouts.push_back(descriptorSetLayout);
		}
		else if (rootParameter.parameterType == RootParameterType::RootSRV)
		{
			VkDescriptorSetLayoutBinding vkBinding{
				.binding				= rootParameter.startRegister + rootSignatureDesc.bindingOffsets.shaderResource,
				.descriptorType			= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount		= 1,
				.stageFlags				= shaderStageFlags,
				.pImmutableSamplers		= nullptr
			};

			hashCode = Utility::HashState(&vkBinding, 1, hashCode);

			VkDescriptorSetLayoutCreateInfo createInfo{
				.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.pNext			= nullptr,
				.flags			= 0,
				.bindingCount	= 1,
				.pBindings		= &vkBinding
			};

			VkDescriptorSetLayout vkDescriptorSetLayout{ VK_NULL_HANDLE };
			vkCreateDescriptorSetLayout(*m_vkDevice, &createInfo, nullptr, &vkDescriptorSetLayout);
			vkDescriptorSetLayouts.push_back(vkDescriptorSetLayout);

			wil::com_ptr<CVkDescriptorSetLayout> descriptorSetLayout = Create<CVkDescriptorSetLayout>(m_vkDevice.get(), vkDescriptorSetLayout);
			descriptorSetLayouts.push_back(descriptorSetLayout);
		}
		else if (rootParameter.parameterType == RootParameterType::RootUAV)
		{
			VkDescriptorSetLayoutBinding vkBinding{
				.binding				= rootParameter.startRegister + rootSignatureDesc.bindingOffsets.unorderedAccess,
				.descriptorType			= VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount		= 1,
				.stageFlags				= shaderStageFlags,
				.pImmutableSamplers		= nullptr
			};

			hashCode = Utility::HashState(&vkBinding, 1, hashCode);

			VkDescriptorSetLayoutCreateInfo createInfo{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.pNext = nullptr,
				.flags = 0,
				.bindingCount = 1,
				.pBindings = &vkBinding
			};

			VkDescriptorSetLayout vkDescriptorSetLayout{ VK_NULL_HANDLE };
			vkCreateDescriptorSetLayout(*m_vkDevice, &createInfo, nullptr, &vkDescriptorSetLayout);
			vkDescriptorSetLayouts.push_back(vkDescriptorSetLayout);

			wil::com_ptr<CVkDescriptorSetLayout> descriptorSetLayout = Create<CVkDescriptorSetLayout>(m_vkDevice.get(), vkDescriptorSetLayout);
			descriptorSetLayouts.push_back(descriptorSetLayout);
		}
		else if (rootParameter.parameterType == RootParameterType::Table)
		{
			std::vector<VkDescriptorSetLayoutBinding> vkLayoutBindings;

			for (const auto& range : rootParameter.table)
			{
				VkDescriptorSetLayoutBinding& vkBinding = vkLayoutBindings.emplace_back();
				vkBinding.descriptorCount = range.numDescriptors;
				vkBinding.descriptorType = DescriptorTypeToVulkan(range.descriptorType);
				vkBinding.stageFlags = shaderStageFlags;
				vkBinding.pImmutableSamplers = nullptr;

				uint32_t offset{ 0 };

				switch (range.descriptorType)
				{
				case DescriptorType::TextureSRV:
				case DescriptorType::StructuredBufferSRV:
				case DescriptorType::TypedBufferSRV:
				case DescriptorType::RawBufferSRV:
				case DescriptorType::RayTracingAccelStruct:
					offset = rootSignatureDesc.bindingOffsets.shaderResource;
					break;

				case DescriptorType::TextureUAV:
				case DescriptorType::StructuredBufferUAV:
				case DescriptorType::TypedBufferUAV:
				case DescriptorType::RawBufferUAV:
				case DescriptorType::SamplerFeedbackTextureUAV:
					offset = rootSignatureDesc.bindingOffsets.unorderedAccess;
					break;

				case DescriptorType::ConstantBuffer:
				case DescriptorType::DynamicConstantBuffer:
					offset = rootSignatureDesc.bindingOffsets.constantBuffer;
					break;

				case DescriptorType::Sampler:
					offset = rootSignatureDesc.bindingOffsets.sampler;
					break;
				}

				vkBinding.binding = range.startRegister + offset;

				hashCode = Utility::HashState(&vkBinding, 1, hashCode);
			}

			VkDescriptorSetLayoutCreateInfo createInfo{
				.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.pNext			= nullptr,
				.flags			= 0,
				.bindingCount	= (uint32_t)vkLayoutBindings.size(),
				.pBindings		= vkLayoutBindings.data()
			};

			VkDescriptorSetLayout vkDescriptorSetLayout{ VK_NULL_HANDLE };
			vkCreateDescriptorSetLayout(*m_vkDevice, &createInfo, nullptr, &vkDescriptorSetLayout);
			vkDescriptorSetLayouts.push_back(vkDescriptorSetLayout);

			wil::com_ptr<CVkDescriptorSetLayout> descriptorSetLayout = Create<CVkDescriptorSetLayout>(m_vkDevice.get(), vkDescriptorSetLayout);
			descriptorSetLayouts.push_back(descriptorSetLayout);
		}
	}

	CVkPipelineLayout** ppPipelineLayout;
	bool firstCompile = false;
	{
		lock_guard<mutex> CS(m_pipelineLayoutMutex);

		auto iter = m_pipelineLayoutHashMap.find(hashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == m_pipelineLayoutHashMap.end())
		{
			ppPipelineLayout = m_pipelineLayoutHashMap[hashCode].addressof();
			firstCompile = true;
		}
		else
		{
			ppPipelineLayout = &iter->second;
		}
	}

	if (firstCompile)
	{
		VkPipelineLayoutCreateInfo createInfo{
			.sType						= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.pNext						= nullptr,
			.flags						= 0,
			.setLayoutCount				= (uint32_t)vkDescriptorSetLayouts.size(),
			.pSetLayouts				= vkDescriptorSetLayouts.data(),
			.pushConstantRangeCount		= (uint32_t)vkPushConstantRanges.size(),
			.pPushConstantRanges		= vkPushConstantRanges.data()
		};

		VkPipelineLayout vkPipelineLayout{ VK_NULL_HANDLE };
		vkCreatePipelineLayout(*m_vkDevice, &createInfo, nullptr, &vkPipelineLayout);

		wil::com_ptr<CVkPipelineLayout> pipelineLayout = Create<CVkPipelineLayout>(m_vkDevice.get(), vkPipelineLayout);

		*ppPipelineLayout = pipelineLayout.get();

		(*ppPipelineLayout)->AddRef();
	}

	RootSignatureDescExt rootSignatureDescExt{
		.pipelineLayout			= *ppPipelineLayout,
		.descriptorSetLayouts	= descriptorSetLayouts
	};

	return Make<RootSignatureVK>(rootSignatureDesc, rootSignatureDescExt);
}


void GraphicsDevice::CreateResources()
{
	m_vmaAllocator = CreateVmaAllocator();
}


wil::com_ptr<CVkFence> GraphicsDevice::CreateFence(bool bSignaled) const
{
	auto createInfo = VkFenceCreateInfo{ 
		.sType		= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext		= nullptr,
		.flags		= bSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0u
	};

	VkFence fence{ VK_NULL_HANDLE };
	if (VK_SUCCEEDED(vkCreateFence(*m_vkDevice, &createInfo, nullptr, &fence)))
	{
		return Create<CVkFence>(m_vkDevice.get(), fence);
	}

	return nullptr;
}


wil::com_ptr<CVkSemaphore> GraphicsDevice::CreateSemaphore(VkSemaphoreType semaphoreType, uint64_t initialValue) const
{
	VkSemaphoreTypeCreateInfo typeCreateInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
	typeCreateInfo.semaphoreType = semaphoreType;
	typeCreateInfo.initialValue = initialValue;

	VkSemaphoreCreateInfo createInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	createInfo.pNext = &typeCreateInfo;

	VkSemaphore semaphore{ VK_NULL_HANDLE };
	if (VK_SUCCEEDED(vkCreateSemaphore(*m_vkDevice, &createInfo, nullptr, &semaphore)))
	{
		return Create<CVkSemaphore>(m_vkDevice.get(), semaphore);
	}

	return nullptr;
}


wil::com_ptr<CVkCommandPool> GraphicsDevice::CreateCommandPool(CommandListType commandListType) const
{
	uint32_t queueFamilyIndex{ 0 };
	switch (commandListType)
	{
	case CommandListType::Compute: 
		queueFamilyIndex = m_desc.queueFamilyIndices.compute; 
		break;

	case CommandListType::Copy: 
		queueFamilyIndex = m_desc.queueFamilyIndices.transfer; 
		break;

	default: 
		queueFamilyIndex = m_desc.queueFamilyIndices.graphics; 
		break;
	}

	VkCommandPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	createInfo.queueFamilyIndex = queueFamilyIndex;

	VkCommandPool vkCommandPool{ VK_NULL_HANDLE };
	if (VK_SUCCEEDED(vkCreateCommandPool(*m_vkDevice, &createInfo, nullptr, &vkCommandPool)))
	{
		return Create<CVkCommandPool>(m_vkDevice.get(), vkCommandPool);
	}

	return nullptr;
}


wil::com_ptr<CVmaAllocator> GraphicsDevice::CreateVmaAllocator() const
{
	VmaVulkanFunctions vmaFunctions{};
	vmaFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vmaFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

	VmaAllocatorCreateInfo createInfo{};
	createInfo.physicalDevice = m_vkDevice->GetPhysicalDevice();
	createInfo.device = *m_vkDevice;
	createInfo.instance = m_desc.instance;
	createInfo.pVulkanFunctions = &vmaFunctions;

	VmaAllocator vmaAllocator{ VK_NULL_HANDLE };
	if (VK_SUCCEEDED(vmaCreateAllocator(&createInfo, &vmaAllocator)))
	{
		return Create<CVmaAllocator>(m_vkDevice.get(), vmaAllocator);
	}
	else
	{
		LogError(LogVulkan) << "Failed to create VmaAllocator.  Error code: " << res << endl;
	}

	return nullptr;
}


wil::com_ptr<CVkImage> GraphicsDevice::CreateImage(const ImageDesc& desc) const
{
	auto imageCreateInfo = VkImageCreateInfo{ 
		.sType			= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.flags			= (VkImageCreateFlags)GetImageCreateFlags(desc.resourceType),
		.imageType		= GetImageType(desc.resourceType),
		.format			= FormatToVulkan(desc.format),
		.extent			= { 
							.width	= (uint32_t)desc.width, 
							.height = desc.height, 
							.depth	= desc.arraySizeOrDepth },
		.mipLevels		= desc.numMips,
		.arrayLayers	= HasAnyFlag(desc.resourceType, ResourceType::TextureArray_Type) ? desc.arraySizeOrDepth : 1,
		.samples		= GetSampleCountFlags(desc.numSamples),
		.tiling			= VK_IMAGE_TILING_OPTIMAL,
		.usage			= GetImageUsageFlags(desc.imageUsage)
	};
	
	// Remove storage flag if this format doesn't support it.
	// TODO - Make a table with all the format properties?
	VkFormatProperties properties = GetFormatProperties(desc.format);
	if (!QueryOptimalTilingFeature(properties, VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT))
	{
		imageCreateInfo.usage &= ~VK_IMAGE_USAGE_STORAGE_BIT;
	}

	VmaAllocationCreateInfo imageAllocCreateInfo{};
	imageAllocCreateInfo.flags = GetMemoryFlags(desc.memoryAccess);
	imageAllocCreateInfo.usage = GetMemoryUsage(desc.memoryAccess);

	VkImage vkImage{ VK_NULL_HANDLE };
	VmaAllocation vmaAllocation{ VK_NULL_HANDLE };
	if (VK_SUCCEEDED(vmaCreateImage(*m_vmaAllocator, &imageCreateInfo, &imageAllocCreateInfo, &vkImage, &vmaAllocation, nullptr)))
	{
		SetDebugName(*m_vkDevice, vkImage, desc.name);

		return Create<CVkImage>(m_vkDevice.get(), m_vmaAllocator.get(), vkImage, vmaAllocation);
	}
	else
	{
		LogError(LogVulkan) << "Failed to create VkImage.  Error code: " << res << endl;
	}

	return nullptr;
}


wil::com_ptr<CVkImageView> GraphicsDevice::CreateImageView(const ImageViewDesc& desc) const
{
	VkImageViewCreateInfo createInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	createInfo.viewType = GetImageViewType(desc.resourceType, desc.imageUsage);
	createInfo.format = FormatToVulkan(desc.format);
	if (IsColorFormat(desc.format))
	{
		createInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_A;
	}
	createInfo.subresourceRange = {};
	createInfo.subresourceRange.aspectMask = GetImageAspect(desc.imageAspect);
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = desc.mipCount;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = desc.resourceType == ResourceType::Texture3D ? 1 : desc.arraySize;
	createInfo.image = desc.image->Get();

	VkImageView vkImageView{ VK_NULL_HANDLE };
	if (VK_SUCCEEDED(vkCreateImageView(*m_vkDevice, &createInfo, nullptr, &vkImageView)))
	{
		return Create<CVkImageView>(m_vkDevice.get(), desc.image, vkImageView);
	}
	else
	{
		LogWarning(LogVulkan) << "Failed to create VkImageView.  Error code: " << res << endl;
	}

	return nullptr;
}


wil::com_ptr<CVkBuffer> GraphicsDevice::CreateStagingBuffer(const void* initialData, size_t numBytes) const
{
	VkBufferCreateInfo stagingBufferInfo{ 
		.sType			= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO ,
		.pNext			= nullptr,
		.size			= numBytes,
		.usage			= VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		.sharingMode	= VK_SHARING_MODE_EXCLUSIVE
	};

	VmaAllocationCreateInfo stagingAllocCreateInfo{
		.flags	= VMA_ALLOCATION_CREATE_MAPPED_BIT,
		.usage	= VMA_MEMORY_USAGE_CPU_ONLY
	};
	
	VkBuffer stagingBuffer{ VK_NULL_HANDLE };
	VmaAllocation stagingBufferAlloc{ VK_NULL_HANDLE };
	VmaAllocationInfo stagingAllocInfo{};

	auto allocator = GetVulkanGraphicsDevice()->GetAllocator();
	vmaCreateBuffer(allocator, &stagingBufferInfo, &stagingAllocCreateInfo, &stagingBuffer, &stagingBufferAlloc, &stagingAllocInfo);

	memcpy(stagingAllocInfo.pMappedData, initialData, numBytes);

	return Create<CVkBuffer>(m_vkDevice.get(), m_vmaAllocator.get(), stagingBuffer, stagingBufferAlloc);
}


VkFormatProperties GraphicsDevice::GetFormatProperties(Format format) const
{
	VkFormat vkFormat = static_cast<VkFormat>(format);
	VkFormatProperties properties{};

	vkGetPhysicalDeviceFormatProperties(m_vkDevice->GetPhysicalDevice(), vkFormat, &properties);

	return properties;
}


GraphicsDevice* GetVulkanGraphicsDevice()
{
	return g_vulkanGraphicsDevice;
}

} // namespace Luna::VK