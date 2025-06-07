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

#include "Graphics\ResourceManager.h"

#include "Graphics\Vulkan\ColorBufferFactoryVK.h"
#include "Graphics\Vulkan\DepthBufferFactoryVK.h"
#include "Graphics\Vulkan\DescriptorSetFactoryVK.h"
#include "Graphics\Vulkan\GpuBufferFactoryVK.h"
#include "Graphics\Vulkan\PipelineStateFactoryVK.h"
#include "Graphics\Vulkan\RootSignatureFactoryVK.h"
#include "Graphics\Vulkan\VulkanCommon.h"


namespace Luna::VK
{

class ResourceManager : public TResourceManager<
	CVkDevice,
	CVmaAllocator,
	ColorBufferFactory,
	DepthBufferFactory,
	GpuBufferFactory,
	PipelineStateFactory,
	RootSignatureFactory,
	DescriptorSetFactory>
{
public:
	ResourceManager(CVkDevice* device, CVmaAllocator* allocator);
	~ResourceManager();

	// Creation/destruction methods
	ResourceHandle CreateDescriptorSet(const DescriptorSetDesc& descriptorSetDesc);

	// Root signature methods
	ResourceHandle CreateDescriptorSet(const ResourceHandleType* handle, uint32_t rootParamIndex) override;

	// Platform specific methods
	ResourceHandle CreateColorBufferFromSwapChainImage(CVkImage* swapChainImage, uint32_t width, uint32_t height, Format format, uint32_t imageIndex);
	VkImage GetImage(const ResourceHandleType* handle) const;
	VkBuffer GetBuffer(const ResourceHandleType* handle) const;
	VkImageView GetImageViewSrv(const ResourceHandleType* handle) const;
	VkImageView GetImageViewRtv(const ResourceHandleType* handle) const;
	VkImageView GetImageViewDepth(const ResourceHandleType* handle, DepthStencilAspect depthStencilAspect) const;
	VkDescriptorImageInfo GetImageInfoSrv(const ResourceHandleType* handle) const;
	VkDescriptorImageInfo GetImageInfoUav(const ResourceHandleType* handle) const;
	VkDescriptorImageInfo GetImageInfoDepth(const ResourceHandleType* handle, bool depthSrv) const;
	VkDescriptorBufferInfo GetBufferInfo(const ResourceHandleType* handle) const;
	VkBufferView GetBufferView(const ResourceHandleType* handle) const;
	VkPipeline GetGraphicsPipeline(const ResourceHandleType* handle) const;
	VkPipelineLayout GetPipelineLayout(const ResourceHandleType* handle) const;
	CVkDescriptorSetLayout* GetDescriptorSetLayout(const ResourceHandleType* handle, uint32_t paramIndex) const;
	const std::vector<DescriptorBindingDesc>& GetLayoutBindings(const ResourceHandleType* handle, uint32_t paramIndex) const;
	bool HasDescriptors(const ResourceHandleType* handle) const;
	VkDescriptorSet GetDescriptorSet(const ResourceHandleType* handle) const;
	uint32_t GetDynamicOffset(const ResourceHandleType* handle) const;
	bool IsDynamicBuffer(const ResourceHandleType* handle) const;

private:
	std::tuple<uint32_t, uint32_t> UnpackHandle(const ResourceHandleType* handle) const;
};


ResourceManager* const GetVulkanResourceManager();

} // namespace Luna::VK