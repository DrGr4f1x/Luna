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

GpuBufferVK::GpuBufferVK(const GpuBufferDesc& gpuBufferDesc, const GpuBufferDescExt& gpuBufferDescExt)
	: m_name{ gpuBufferDesc.name }
	, m_resourceType{ gpuBufferDesc.resourceType }
	, m_usageState{ gpuBufferDescExt.usageState }
	, m_elementCount{ gpuBufferDesc.elementCount }
	, m_elementSize{ gpuBufferDesc.elementSize }
	, m_buffer{ gpuBufferDescExt.buffer }
	, m_bufferView{ gpuBufferDescExt.bufferView }
	, m_bufferInfo{ gpuBufferDescExt.bufferInfo }
{}


NativeObjectPtr GpuBufferVK::GetNativeObject(NativeObjectType type, uint32_t index) const noexcept
{
	using enum NativeObjectType;

	switch (type)
	{
	case VK_Buffer:
		return NativeObjectPtr(m_buffer->Get());

	case VK_BufferInfo:
		return NativeObjectPtr(GetDescriptorBufferInfo());

	case VK_BufferView:
		return NativeObjectPtr(m_bufferView->Get());

	default:
		assert(false);
		return nullptr;
	}
}


void GpuBufferVK::Update(size_t sizeInBytes, const void* data)
{
	Update(sizeInBytes, 0, data);
}


void GpuBufferVK::Update(size_t sizeInBytes, size_t offset, const void* data)
{
	assert((sizeInBytes + offset) <= GetSize());

	// Map uniform buffer and update it
	uint8_t* pData = nullptr;
	ThrowIfFailed(vmaMapMemory(m_buffer->GetAllocator(), m_buffer->GetAllocation(), (void**)&pData));
	memcpy(pData + offset, data, sizeInBytes);
	// Unmap after data has been copied
	// Note: Since we requested a host coherent memory type for the uniform buffer, the write is instantly visible to the GPU
	vmaUnmapMemory(m_buffer->GetAllocator(), m_buffer->GetAllocation());
}

} // namespace Luna::VK