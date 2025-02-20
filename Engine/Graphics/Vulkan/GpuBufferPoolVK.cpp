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

#include "GpuBufferPoolVK.h"


namespace Luna::VK
{

GpuBufferPool* g_gpuBufferPool{ nullptr };


GpuBufferPool::GpuBufferPool(CVkDevice* device, CVmaAllocator* allocator)
	: m_device{ device }
	, m_allocator{ allocator }
{
	assert(g_gpuBufferPool == nullptr);

	for (uint32_t i = 0; i < MaxItems; ++i)
	{
		m_freeList.push(i);
		m_descs[i] = GpuBufferDesc{};
		m_gpuBufferData[i] = GpuBufferData{};
	}

	g_gpuBufferPool = this;
}


GpuBufferPool::~GpuBufferPool()
{
	g_gpuBufferPool = nullptr;
}


GpuBufferHandle GpuBufferPool::CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc)
{
	std::lock_guard guard(m_allocationMutex);

	assert(!m_freeList.empty());

	uint32_t index = m_freeList.front();
	m_freeList.pop();

	m_descs[index] = gpuBufferDesc;

	m_gpuBufferData[index] = CreateBuffer_Internal(gpuBufferDesc);

	return Create<GpuBufferHandleType>(index, this);
}


void GpuBufferPool::DestroyHandle(GpuBufferHandleType* handle)
{
	assert(handle != nullptr);

	std::lock_guard guard(m_allocationMutex);

	// TODO: queue this up to execute in one big batch, e.g. at the end of the frame

	uint32_t index = handle->GetIndex();
	m_descs[index] = GpuBufferDesc{};
	m_gpuBufferData[index] = GpuBufferData{};

	m_freeList.push(index);
}


ResourceType GpuBufferPool::GetResourceType(GpuBufferHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_descs[index].resourceType;
}


ResourceState GpuBufferPool::GetUsageState(GpuBufferHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_gpuBufferData[index].usageState;
}


void GpuBufferPool::SetUsageState(GpuBufferHandleType* handle, ResourceState newState)
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	m_gpuBufferData[index].usageState = newState;
}


size_t GpuBufferPool::GetSize(GpuBufferHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_descs[index].elementSize * m_descs[index].elementCount;
}


size_t GpuBufferPool::GetElementCount(GpuBufferHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_descs[index].elementCount;
}


size_t GpuBufferPool::GetElementSize(GpuBufferHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_descs[index].elementSize;
}


void GpuBufferPool::Update(GpuBufferHandleType* handle, size_t sizeInBytes, size_t offset, const void* data) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();

	assert((sizeInBytes + offset) <= (m_descs[index].elementSize * m_descs[index].elementCount));
	assert(HasFlag(m_descs[index].memoryAccess, MemoryAccess::CpuWrite));

	CVkBuffer* buffer = m_gpuBufferData[index].buffer.get();

	// Map uniform buffer and update it
	uint8_t* pData = nullptr;
	ThrowIfFailed(vmaMapMemory(buffer->GetAllocator(), buffer->GetAllocation(), (void**)&pData));
	memcpy(pData + offset, data, sizeInBytes);
	// Unmap after data has been copied
	// Note: Since we requested a host coherent memory type for the uniform buffer, the write is instantly visible to the GPU
	vmaUnmapMemory(buffer->GetAllocator(), buffer->GetAllocation());
}


VkBuffer GpuBufferPool::GetBuffer(GpuBufferHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_gpuBufferData[index].buffer->Get();
}


VkDescriptorBufferInfo GpuBufferPool::GetBufferInfo(GpuBufferHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_gpuBufferData[index].bufferInfo;
}


VkBufferView GpuBufferPool::GetBufferView(GpuBufferHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_gpuBufferData[index].bufferView->Get();
}


GpuBufferData GpuBufferPool::CreateBuffer_Internal(const GpuBufferDesc& gpuBufferDesc) const
{
	constexpr VkBufferUsageFlags transferFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	VkBufferCreateInfo bufferCreateInfo{
		.sType	= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size	= gpuBufferDesc.elementCount * gpuBufferDesc.elementSize,
		.usage	= GetBufferUsageFlags(gpuBufferDesc.resourceType) | transferFlags
	};

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.flags = GetMemoryFlags(gpuBufferDesc.memoryAccess);
	allocCreateInfo.usage = GetMemoryUsage(gpuBufferDesc.memoryAccess);

	VkBuffer vkBuffer{ VK_NULL_HANDLE };
	VmaAllocation vmaBufferAllocation{ VK_NULL_HANDLE };

	auto res = vmaCreateBuffer(*m_allocator, &bufferCreateInfo, &allocCreateInfo, &vkBuffer, &vmaBufferAllocation, nullptr);
	assert(res == VK_SUCCESS);

	SetDebugName(*m_device, vkBuffer, gpuBufferDesc.name);

	auto buffer = Create<CVkBuffer>(m_device.get(), m_allocator.get(), vkBuffer, vmaBufferAllocation);

	wil::com_ptr<CVkBufferView> bufferView;
	if (gpuBufferDesc.resourceType == ResourceType::TypedBuffer)
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

	GpuBufferData gpuBufferData{
		.buffer			= buffer,
		.bufferView		= bufferView,
		.bufferInfo = {
			.buffer		= vkBuffer,
			.offset		= 0,
			.range		= VK_WHOLE_SIZE
		},
		.usageState		= ResourceState::Common
	};

	return gpuBufferData;
}


GpuBufferPool* const GetVulkanGpuBufferPool()
{
	return g_gpuBufferPool;
}

} // namespace Luna::VK