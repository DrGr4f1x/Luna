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
#include "Graphics\Vulkan\RefCountingImplVK.h"
#include "Graphics\Vulkan\VulkanUtil.h"


namespace Luna
{

// Forward declarations
class IResourceManager;

} // namespace Luna


namespace Luna::VK
{

struct GpuBufferData
{
	wil::com_ptr<CVkBufferView> bufferView;
	VkDescriptorBufferInfo bufferInfo;
};


class GpuBufferFactory : public GpuBufferFactoryBase
{
public:
	GpuBufferFactory(IResourceManager* owner, CVkDevice* device, CVmaAllocator* allocator);

	ResourceHandle CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc);
	void Destroy(uint32_t index);

	// General resource methods
	ResourceType GetResourceType(uint32_t index) const
	{
		return m_descs[index].resourceType;
	}


	ResourceState GetUsageState(uint32_t index) const
	{
		return m_buffers[index].usageState;
	}


	void SetUsageState(uint32_t index, ResourceState newState)
	{
		m_buffers[index].usageState = newState;
	}


	VkBuffer GetBuffer(uint32_t index) const
	{
		return m_buffers[index].buffer->Get();
	}


	VkDescriptorBufferInfo GetBufferInfo(uint32_t index) const
	{
		return m_data[index].bufferInfo;
	}


	VkBufferView GetBufferView(uint32_t index) const
	{
		return m_data[index].bufferView->Get();
	}

	void Update(uint32_t index, size_t sizeInBytes, size_t offset, const void* data) const;

private:
	void ResetBuffer(uint32_t index);
	void ResetData(uint32_t index);

	void ClearBuffers();
	void ClearData();

private:
	IResourceManager* m_owner{ nullptr };
	wil::com_ptr<CVkDevice> m_device;
	wil::com_ptr<CVmaAllocator> m_allocator;

	std::mutex m_mutex;

	std::queue<uint32_t> m_freeList;

	std::array<BufferData, MaxResources> m_buffers;
	std::array<GpuBufferData, MaxResources> m_data;
};

} // namespace Luna::VK