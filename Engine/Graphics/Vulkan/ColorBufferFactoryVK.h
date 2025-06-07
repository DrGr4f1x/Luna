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

#include "Graphics\ColorBuffer.h"
#include "Graphics\Vulkan\RefCountingImplVK.h"
#include "Graphics\Vulkan\VulkanUtil.h"


namespace Luna
{

// Forward declarations
class IResourceManager;

} // namespace Luna


namespace Luna::VK
{

struct ColorBufferData
{
	wil::com_ptr<CVkImageView> imageViewRtv{ nullptr };
	wil::com_ptr<CVkImageView> imageViewSrv{ nullptr };
	VkDescriptorImageInfo imageInfoSrv{};
	VkDescriptorImageInfo imageInfoUav{};
	uint32_t planeCount{ 1 };
};


class ColorBufferFactory : public ColorBufferFactoryBase
{
public:
	ColorBufferFactory(IResourceManager* owner, CVkDevice* device, CVmaAllocator* allocator);

	ResourceHandle CreateColorBuffer(const ColorBufferDesc& colorBufferDesc);
	void Destroy(uint32_t index);

	ResourceHandle CreateColorBufferFromSwapChainImage(CVkImage* swapChainImage, uint32_t width, uint32_t height, Format format, uint32_t imageIndex);

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


	VkImageView GetImageViewSrv(uint32_t index) const
	{
		return m_data[index].imageViewSrv->Get();
	}


	VkImageView GetImageViewRtv(uint32_t index) const
	{
		return m_data[index].imageViewRtv->Get();
	}


	VkDescriptorImageInfo GetImageInfoSrv(uint32_t index) const
	{
		return m_data[index].imageInfoSrv;
	}


	VkDescriptorImageInfo GetImageInfoUav(uint32_t index) const
	{
		return m_data[index].imageInfoUav;
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
	std::array<ColorBufferData, MaxResources> m_data;
};

} // namespace Luna::VK
