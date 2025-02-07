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

#include "GpuBuffer12.h"

#include "Device12.h"


namespace Luna::DX12
{

GpuBuffer12::GpuBuffer12(const GpuBufferDesc& gpuBufferDesc, const GpuBufferDescExt& gpuBufferDescExt)
	: m_name{ gpuBufferDesc.name }
	, m_resourceType{ gpuBufferDesc.resourceType }
	, m_usageState{ gpuBufferDescExt.usageState }
	, m_memoryAccess{ gpuBufferDesc.memoryAccess }
	, m_elementCount{ gpuBufferDesc.elementCount }
	, m_elementSize{ gpuBufferDesc.elementSize }
	, m_resource{ gpuBufferDescExt.resource }
	, m_allocation{ gpuBufferDescExt.allocation }
	, m_srvHandle{ gpuBufferDescExt.srvHandle }
	, m_uavHandle{ gpuBufferDescExt.uavHandle }
	, m_cbvHandle{ gpuBufferDescExt.cbvHandle }
{}


NativeObjectPtr GpuBuffer12::GetNativeObject(NativeObjectType type, uint32_t index) const noexcept
{
	using enum NativeObjectType;

	switch (type)
	{
	case DX12_Resource:
		return NativeObjectPtr(m_resource.get());

	case DX12_SRV:
		return NativeObjectPtr(m_srvHandle.ptr);

	case DX12_UAV:
		return NativeObjectPtr(m_uavHandle.ptr);

	case DX12_GpuVirtualAddress:
		return NativeObjectPtr(m_resource->GetGPUVirtualAddress());

	case DX12_CBV:
		return NativeObjectPtr(m_cbvHandle.ptr);

	default:
		assert(false);
		return nullptr;
	}
}


void GpuBuffer12::Update(size_t sizeInBytes, const void* data)
{
	Update(sizeInBytes, 0, data);
}


void GpuBuffer12::Update(size_t sizeInBytes, size_t offset, const void* data)
{
	assert((sizeInBytes + offset) <= GetSize());
	assert(HasFlag(m_memoryAccess, MemoryAccess::CpuWrite));

	CD3DX12_RANGE readRange(0, 0);

	// Map uniform buffer and update it
	uint8_t* pData = nullptr;
	ThrowIfFailed(m_resource->Map(0, &readRange, reinterpret_cast<void**>(&pData)));
	memcpy((void*)(pData + offset), data, sizeInBytes);
	// Unmap after data has been copied
	// Note: Since we requested a host coherent memory type for the uniform buffer, the write is instantly visible to the GPU
	m_resource->Unmap(0, nullptr);
}

} // namespace Luna::DX12