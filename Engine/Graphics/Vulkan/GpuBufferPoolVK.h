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

#include "Graphics\GpuBuffer.h"
#include "Graphics\Vulkan\VulkanCommon.h"


namespace Luna::VK
{

struct GpuBufferData
{
	wil::com_ptr<CVkBuffer> buffer;
	wil::com_ptr<CVkBufferView> bufferView;
	VkDescriptorBufferInfo bufferInfo;
	// TODO: Use Vulkan type(s) directly
	ResourceState usageState{ ResourceState::Undefined };
};


class GpuBufferPool : public IGpuBufferPool
{
	static const uint32_t MaxItems = (1 << 12);

public:
	explicit GpuBufferPool(CVkDevice* device, CVmaAllocator* allocator);
	~GpuBufferPool();

	// Create/Destroy GpuBuffer
	GpuBufferHandle CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc) override;
	void DestroyHandle(GpuBufferHandleType* handle) override;

	// Platform agnostic functions
	ResourceType GetResourceType(GpuBufferHandleType* handle) const override;
	ResourceState GetUsageState(GpuBufferHandleType* handle) const override;
	void SetUsageState(GpuBufferHandleType* handle, ResourceState newState) override;
	size_t GetSize(GpuBufferHandleType* handle) const override;
	size_t GetElementCount(GpuBufferHandleType* handle) const override;
	size_t GetElementSize(GpuBufferHandleType* handle) const override;
	void Update(GpuBufferHandleType* handle, size_t sizeInBytes, size_t offset, const void* data) const override;

	// Platform specific functions
	// TODO: Figure out a better way to do this.  Returning VkDescriptorBufferInfo by address isn't great.
	VkBuffer GetBuffer(GpuBufferHandleType* handle) const;
	VkDescriptorBufferInfo GetBufferInfo(GpuBufferHandleType* handle) const;
	VkBufferView GetBufferView(GpuBufferHandleType* handle) const;

private:
	GpuBufferData CreateBuffer_Internal(const GpuBufferDesc& gpuBufferDesc) const;

private:
	wil::com_ptr<CVkDevice> m_device;
	wil::com_ptr<CVmaAllocator> m_allocator;

	// Allocation mutex
	std::mutex m_allocationMutex;

	// Free list
	std::queue<uint32_t> m_freeList;

	// Cold data
	std::array<GpuBufferDesc, MaxItems> m_descs;

	// Hot data
	std::array<GpuBufferData, MaxItems> m_gpuBufferData;
};


GpuBufferPool* const GetVulkanGpuBufferPool();

} // namespace Luna::VK