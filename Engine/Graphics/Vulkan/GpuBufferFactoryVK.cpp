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

#include "GpuBufferFactoryVK.h"

#include "Graphics\ResourceManager.h"


namespace Luna::VK
{

GpuBufferFactory::GpuBufferFactory(IResourceManager* owner, CVkDevice* device, CVmaAllocator* allocator)
	: m_owner{ owner }
	, m_device{ device }
	, m_allocator{ allocator }
{
	for (uint32_t i = 0; i < MaxResources; ++i)
	{
		m_freeList.push(i);
	}

	ClearBuffers();
	ClearData();
}


ResourceHandle GpuBufferFactory::CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc)
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

	BufferData bufferData{
		.buffer			= buffer,
		.usageState		= ResourceState::Common
	};

	GpuBufferData gpuBufferData{
		.bufferView = bufferView,
		.bufferInfo = {.buffer = vkBuffer, .offset = 0, .range = VK_WHOLE_SIZE	}
	};

	// Create handle and store cached data
	{
		std::lock_guard lock(m_mutex);

		assert(!m_freeList.empty());

		// Get an index allocation
		uint32_t index = m_freeList.front();
		m_freeList.pop();

		m_descs[index] = gpuBufferDesc;
		m_buffers[index] = bufferData;
		m_data[index] = gpuBufferData;

		return std::make_shared<ResourceHandleType>(index, IResourceManager::ManagedGpuBuffer, m_owner);
	}
}


void GpuBufferFactory::Destroy(uint32_t index)
{
	std::lock_guard lock(m_mutex);

	ResetDesc(index);
	ResetBuffer(index);
	ResetData(index);

	m_freeList.push(index);
}


void GpuBufferFactory::Update(uint32_t index, size_t sizeInBytes, size_t offset, const void* data) const
{
	assert((sizeInBytes + offset) <= (m_descs[index].elementSize * m_descs[index].elementCount));
	assert(HasFlag(m_descs[index].memoryAccess, MemoryAccess::CpuWrite));

	CVkBuffer* buffer = m_buffers[index].buffer.get();

	// Map uniform buffer and update it
	uint8_t* pData = nullptr;
	ThrowIfFailed(vmaMapMemory(buffer->GetAllocator(), buffer->GetAllocation(), (void**)&pData));
	memcpy(pData + offset, data, sizeInBytes);
	// Unmap after data has been copied
	// Note: Since we requested a host coherent memory type for the uniform buffer, the write is instantly visible to the GPU
	vmaUnmapMemory(buffer->GetAllocator(), buffer->GetAllocation());
}


void GpuBufferFactory::ResetBuffer(uint32_t index)
{
	m_buffers[index] = BufferData{};
}


void GpuBufferFactory::ResetData(uint32_t index)
{
	m_data[index] = GpuBufferData{};
}


void GpuBufferFactory::ClearBuffers()
{
	for (uint32_t i = 0; i < MaxResources; ++i)
	{
		ResetBuffer(i);
	}
}


void GpuBufferFactory::ClearData()
{
	for (uint32_t i = 0; i < MaxResources; ++i)
	{
		ResetData(i);
	}
}

} // namespace Luna::VK