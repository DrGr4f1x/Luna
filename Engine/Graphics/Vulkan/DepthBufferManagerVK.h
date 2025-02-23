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


namespace Luna::VK
{

struct DepthBufferData
{
	wil::com_ptr<CVkImage> image;
	wil::com_ptr<CVkImageView> imageViewDepthStencil;
	wil::com_ptr<CVkImageView> imageViewDepthOnly;
	wil::com_ptr<CVkImageView> imageViewStencilOnly;
	VkDescriptorImageInfo imageInfoDepth{};
	VkDescriptorImageInfo imageInfoStencil{};
	ResourceState usageState{ ResourceState::Common };
	uint32_t planeCount{ 1 };
};


class DepthBufferManager : public IDepthBufferManager
{
	static const uint32_t MaxItems = (1 << 8);

public:
	explicit DepthBufferManager(CVkDevice* device, CVmaAllocator* allocator);
	~DepthBufferManager();

	// Create/Destroy DepthBuffer
	DepthBufferHandle CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc) override;
	void DestroyHandle(DepthBufferHandleType* handle) override;

	// Platform agnostic functions
	ResourceType GetResourceType(DepthBufferHandleType* handle) const;
	ResourceState GetUsageState(DepthBufferHandleType* handle) const;
	void SetUsageState(DepthBufferHandleType* handle, ResourceState newState);
	uint64_t GetWidth(DepthBufferHandleType* handle) const;
	uint32_t GetHeight(DepthBufferHandleType* handle) const;
	uint32_t GetDepthOrArraySize(DepthBufferHandleType* handle) const;
	uint32_t GetNumMips(DepthBufferHandleType* handle) const;
	uint32_t GetNumSamples(DepthBufferHandleType* handle) const;
	uint32_t GetPlaneCount(DepthBufferHandleType* handle) const;
	Format GetFormat(DepthBufferHandleType* handle) const;
	float GetClearDepth(DepthBufferHandleType* handle) const;
	uint8_t GetClearStencil(DepthBufferHandleType* handle) const;

	// Platform specific functions
	VkImage GetImage(DepthBufferHandleType* handle) const;
	VkImageView GetImageView(DepthBufferHandleType* handle, DepthStencilAspect depthStencilAspect) const;
	VkDescriptorImageInfo GetImageInfo(DepthBufferHandleType* handle, bool depthSrv) const;

private:
	const DepthBufferDesc& GetDesc(DepthBufferHandleType* handle) const;
	const DepthBufferData& GetData(DepthBufferHandleType* handle) const;
	DepthBufferData CreateDepthBuffer_Internal(const DepthBufferDesc& depthBufferDesc);

private:
	wil::com_ptr<CVkDevice> m_device;
	wil::com_ptr<CVmaAllocator> m_allocator;

	// Allocation mutex
	std::mutex m_allocationMutex;

	// Free list
	std::queue<uint32_t> m_freeList;

	// Cold data
	std::array<DepthBufferDesc, MaxItems> m_descs;

	// Hot data
	std::array<DepthBufferData, MaxItems> m_depthBufferData;
};


DepthBufferManager* const GetVulkanDepthBufferManager();

} // namespace Luna::VK