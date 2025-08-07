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


namespace Luna::DX12
{

static_assert(sizeof(DrawIndirectArgs) == sizeof(D3D12_DRAW_ARGUMENTS));
static_assert(sizeof(DrawIndexedIndirectArgs) == sizeof(D3D12_DRAW_INDEXED_ARGUMENTS));
static_assert(sizeof(DispatchIndirectArgs) == sizeof(D3D12_DISPATCH_ARGUMENTS));


GpuBuffer::GpuBuffer(Device* device)
{
	m_srvDescriptor.SetDevice(device);
	m_uavDescriptor.SetDevice(device);
	m_cbvDescriptor.SetDevice(device);
}


void GpuBuffer::Update(size_t sizeInBytes, const void* data)
{
	Update(sizeInBytes, 0, data);
}


void GpuBuffer::Update(size_t sizeInBytes, size_t offset, const void* data)
{
	assert((sizeInBytes + offset) <= GetBufferSize());
	assert(m_isCpuWriteable);

	CD3DX12_RANGE readRange(0, 0);

	ID3D12Resource* resource = m_allocation->GetResource();

	// Map uniform buffer and update it
	uint8_t* pData = nullptr;
	ThrowIfFailed(resource->Map(0, &readRange, reinterpret_cast<void**>(&pData)));
	memcpy((void*)(pData + offset), data, sizeInBytes);
	// Unmap after data has been copied
	// Note: Since we requested a host coherent memory type for the uniform buffer, the write is instantly visible to the GPU
	resource->Unmap(0, nullptr);
}


void* GpuBuffer::Map()
{
	void* mem = nullptr;
	auto range = CD3DX12_RANGE(0, GetBufferSize());
	m_allocation->GetResource()->Map(0, &range, &mem);
	return mem;
}


void GpuBuffer::Unmap()
{
	auto range = CD3DX12_RANGE(0, 0);
	m_allocation->GetResource()->Unmap(0, &range);
}


uint64_t GpuBuffer::GetGpuAddress() const noexcept
{
	if (m_allocation && m_allocation->GetResource())
	{
		return m_allocation->GetResource()->GetGPUVirtualAddress();
	}

	return D3D12_GPU_VIRTUAL_ADDRESS_NULL;
}

} // namespace Luna::DX12