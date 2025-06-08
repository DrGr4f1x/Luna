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

#include "Graphics\DepthBuffer.h"
#include "Graphics\Vulkan\VulkanCommon.h"


namespace Luna
{

// Forward declarations
class IResourceManager;

} // namespace Luna


namespace Luna::VK
{

struct DepthBufferData
{
	wil::com_ptr<CVkImageView> imageViewDepthStencil;
	wil::com_ptr<CVkImageView> imageViewDepthOnly;
	wil::com_ptr<CVkImageView> imageViewStencilOnly;
	VkDescriptorImageInfo imageInfoDepth{};
	VkDescriptorImageInfo imageInfoStencil{};
	uint32_t planeCount{ 1 };
};


class DepthBufferFactory : public DepthBufferFactoryBase
{
public:
	DepthBufferFactory(IResourceManager* owner, CVkDevice* device, CVmaAllocator* allocator);

	ResourceHandle CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc);
	void Destroy(uint32_t index);

	VkImageView GetImageView(uint32_t index, DepthStencilAspect depthStencilAspect) const;
	VkDescriptorImageInfo GetImageInfo(uint32_t index, bool depthSrv) const;

	// General resource methods
	ResourceType GetResourceType(uint32_t index) const
	{
		return m_descs[index].resourceType;
	}


	ResourceState GetUsageState(uint32_t index) const
	{
		return m_images[index].usageState;
	}


	void SetUsageState(uint32_t index, ResourceState newState)
	{
		m_images[index].usageState = newState;
	}


	VkImage GetImage(uint32_t index) const
	{
		return m_images[index].image->Get();
	}

private:
	void ResetImage(uint32_t index);
	void ResetData(uint32_t index);

	void ClearImages();
	void ClearData();

private:
	IResourceManager* m_owner{ nullptr };
	wil::com_ptr<CVkDevice> m_device;
	wil::com_ptr<CVmaAllocator> m_allocator;

	std::mutex m_mutex;

	std::queue<uint32_t> m_freeList;

	std::array<ImageData, MaxResources> m_images;
	std::array<DepthBufferData, MaxResources> m_data;
};

} // namespace Luna::VK