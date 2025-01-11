//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#pragma once

#include "Graphics\Vulkan\VulkanCommon.h"


namespace Luna::VK
{

VkBlendFactor BlendToVulkan(Blend blend);

VkBlendOp BlendOpToVulkan(BlendOp blendOp);

VkLogicOp LogicOpToVulkan(LogicOp logicOp);

VkColorComponentFlags ColorWriteToVulkan(ColorWrite colorWrite);

VkCullModeFlags CullModeToVulkan(CullMode cullMode);

VkPolygonMode FillModeToVulkan(FillMode fillMode);

VkCompareOp ComparisonFuncToVulkan(ComparisonFunc comparisonFunc);

VkStencilOp StencilOpToVulkan(StencilOp stencilOp);

VkPrimitiveTopology PrimitiveTopologyToVulkan(PrimitiveTopology primitiveTopology);

uint32_t GetControlPointCount(PrimitiveTopology primitiveTopology);

VkVertexInputRate InputClassificationToVulkan(InputClassification inputClassification);

VkShaderStageFlags ShaderStageToVulkan(ShaderStage shaderStage);

VkDescriptorType DescriptorTypeToVulkan(DescriptorType descriptorType);


enum class SamplerReductionMode
{
	None,
	Min,
	Max
};


enum class TextureSubresourceViewType : uint8_t
{
	AllAspects,
	DepthOnly,
	StencilOnly
};


struct VkTextureFilterMapping
{
	TextureFilter engineFilter;
	VkFilter minFilter;
	VkFilter magFilter;
	VkSamplerMipmapMode mipFilter;
	SamplerReductionMode reductionMode;
	VkBool32 isAnisotropic;
	VkBool32 isComparisonEnabled;
};

VkTextureFilterMapping TextureFilterToVulkan(TextureFilter textureFilter);

VkSamplerAddressMode TextureAddressToVulkan(TextureAddress textureAddress);

VkQueryType QueryTypeToVulkan(QueryType queryHeapType);

AdapterType VkPhysicalDeviceTypeToEngine(VkPhysicalDeviceType physicalDeviceType);

VkImageViewType GetImageViewType(ResourceType type, GpuImageUsage imageUsage);

VkImageViewType GetImageViewType(TextureDimension dimension);

VkImageAspectFlags GetImageAspect(ImageAspect imageAspect);

VkImageAspectFlags GuessSubresourceImageAspect(Format format, TextureSubresourceViewType viewType);

VkImageCreateFlagBits GetImageCreateFlags(ResourceType type);

VkImageType GetImageType(ResourceType type);

VkSampleCountFlagBits GetSampleCountFlags(uint32_t numSamples);

VkImageUsageFlags GetImageUsageFlags(GpuImageUsage usage);

VkBufferUsageFlags GetBufferUsageFlags(ResourceType resourceType);

VmaAllocationCreateFlags GetMemoryFlags(MemoryAccess access);

VmaMemoryUsage GetMemoryUsage(MemoryAccess access);

VkImageLayout GetImageLayout(ResourceState resourceState);

VkAccessFlagBits2 GetAccessMask(ResourceState resourceState);

VkPipelineStageFlagBits2 GetPipelineStage(ResourceState resourceState);

} // namespace Luna::VK