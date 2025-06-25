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

#include "GpuBufferVK.h"


namespace Luna::VK
{

void GpuBuffer::Update(size_t sizeInBytes, const void* data)
{
	Update(sizeInBytes, 0, data);
}


void GpuBuffer::Update(size_t sizeInBytes, size_t offset, const void* data)
{
	assert((sizeInBytes + offset) <= GetBufferSize());
	assert(m_isCpuWriteable);

	// Map uniform buffer and update it
	uint8_t* pData = nullptr;
	ThrowIfFailed(vmaMapMemory(m_buffer->GetAllocator(), m_buffer->GetAllocation(), (void**)&pData));
	memcpy(pData + offset, data, sizeInBytes);
	// Unmap after data has been copied
	// Note: Since we requested a host coherent memory type for the uniform buffer, the write is instantly visible to the GPU
	vmaUnmapMemory(m_buffer->GetAllocator(), m_buffer->GetAllocation());
}


VkBuffer GpuBuffer::GetBuffer() const
{
	return m_buffer ? m_buffer->Get() : VK_NULL_HANDLE;
}


VkBufferView GpuBuffer::GetBufferView() const
{
	return m_bufferView ? m_bufferView->Get() : VK_NULL_HANDLE;
}


VkDescriptorBufferInfo GpuBuffer::GetBufferInfo() const
{
	return m_bufferInfo;
}

} // namespace Luna::VK