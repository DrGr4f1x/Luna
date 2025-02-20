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
#include "Graphics\Vulkan\VulkanCommon.h"

namespace Luna::VK
{

struct ColorBufferData
{
	wil::com_ptr<CVkImage> image{ nullptr };
	wil::com_ptr<CVkImageView> imageViewRtv{ nullptr };
	wil::com_ptr<CVkImageView> imageViewSrv{ nullptr };
	VkDescriptorImageInfo imageInfoSrv{};
	VkDescriptorImageInfo imageInfoUav{};
	uint32_t planeCount{ 1 };
	ResourceState usageState{ ResourceState::Undefined };
};


class ColorBufferPool : public IColorBufferPool
{
	static const uint32_t MaxItems = (1 << 8);

public:
	explicit ColorBufferPool(CVkDevice* device, CVmaAllocator* allocator);
	~ColorBufferPool();

	// Create/Destroy ColorBuffer
	ColorBufferHandle CreateColorBuffer(const ColorBufferDesc& colorBufferDesc) override;
	void DestroyHandle(ColorBufferHandleType* handle) override;

	// Platform agnostic functions
	ResourceType GetResourceType(ColorBufferHandleType* handle) const override;
	ResourceState GetUsageState(ColorBufferHandleType* handle) const override;
	void SetUsageState(ColorBufferHandleType* handle, ResourceState newState) override;
	uint64_t GetWidth(ColorBufferHandleType* handle) const override;
	uint32_t GetHeight(ColorBufferHandleType* handle) const override;
	uint32_t GetDepthOrArraySize(ColorBufferHandleType* handle) const override;
	uint32_t GetNumMips(ColorBufferHandleType* handle) const override;
	uint32_t GetNumSamples(ColorBufferHandleType* handle) const override;
	uint32_t GetPlaneCount(ColorBufferHandleType* handle) const override;
	Format GetFormat(ColorBufferHandleType* handle) const override;
	Color GetClearColor(ColorBufferHandleType* handle) const override;

	// Platform specific functions
	ColorBufferHandle CreateColorBufferFromSwapChainImage(CVkImage* swapChainImage, uint32_t width, uint32_t height, Format format, uint32_t imageIndex);
	VkImage GetImage(ColorBufferHandleType* handle) const;
	VkImageView GetImageViewSrv(ColorBufferHandleType* handle) const;
	VkImageView GetImageViewRtv(ColorBufferHandleType* handle) const;
	VkDescriptorImageInfo GetImageInfoSrv(ColorBufferHandleType* handle) const;
	VkDescriptorImageInfo GetImageInfoUav(ColorBufferHandleType* handle) const;

private:
	const ColorBufferDesc& GetDesc(ColorBufferHandleType* handle) const;
	const ColorBufferData& GetData(ColorBufferHandleType* handle) const;
	ColorBufferData CreateColorBuffer_Internal(const ColorBufferDesc& colorBufferDesc);

private:
	wil::com_ptr<CVkDevice> m_device;
	wil::com_ptr<CVmaAllocator> m_allocator;

	// Allocation mutex
	std::mutex m_allocationMutex;

	// Free list
	std::queue<uint32_t> m_freeList;

	// Cold data
	std::array<ColorBufferDesc, MaxItems> m_descs;

	// Hot data
	std::array<ColorBufferData, MaxItems> m_colorBufferData;
};


ColorBufferPool* const GetVulkanColorBufferPool();

} // namespace Luna::VK