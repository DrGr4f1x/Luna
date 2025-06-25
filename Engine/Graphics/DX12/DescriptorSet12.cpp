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

#include "DescriptorSet12.h"

#include "ColorBuffer12.h"
#include "DepthBuffer12.h"
#include "Device12.h"
#include "GpuBuffer12.h"


namespace Luna::DX12
{

void DescriptorSet::SetSRV(uint32_t slot, const IColorBuffer* colorBuffer)
{
	// TODO: Try this with GetPlatformObject()

	const ColorBuffer* colorBuffer12 = (const ColorBuffer*)colorBuffer;
	assert(colorBuffer12 != nullptr);

	auto descriptor = colorBuffer12->GetSrvHandle();
	SetDescriptor(slot, descriptor);
}


void DescriptorSet::SetSRV(uint32_t slot, const IDepthBuffer* depthBuffer, bool depthSrv)
{
	// TODO: Try this with GetPlatformObject()

	const DepthBuffer* depthBuffer12 = (const DepthBuffer*)depthBuffer;
	assert(depthBuffer12 != nullptr);

	auto descriptor = depthBuffer12->GetSrvHandle(depthSrv);
	SetDescriptor(slot, descriptor);
}


void DescriptorSet::SetSRV(uint32_t slot, const IGpuBuffer* gpuBuffer)
{
	// TODO: Try this with GetPlatformObject()

	const GpuBuffer* gpuBuffer12 = (const GpuBuffer*)gpuBuffer;
	assert(gpuBuffer12 != nullptr);

	if (m_isRootBuffer)
	{
		assert(slot == 0);
		m_gpuAddress = gpuBuffer12->GetGpuAddress();
	}
	else
	{
		auto descriptor = gpuBuffer12->GetSrvHandle();
		SetDescriptor(slot, descriptor);
	}
}


void DescriptorSet::SetUAV(uint32_t slot, const IColorBuffer* colorBuffer, uint32_t uavIndex)
{
	// TODO: Try this with GetPlatformObject()

	const ColorBuffer* colorBuffer12 = (const ColorBuffer*)colorBuffer;
	assert(colorBuffer12 != nullptr);

	auto descriptor = colorBuffer12->GetUavHandle(uavIndex);
	SetDescriptor(slot, descriptor);
}


void DescriptorSet::SetUAV(uint32_t slot, const IDepthBuffer* depthBuffer)
{
	// TODO: Try this with GetPlatformObject()

	const DepthBuffer* depthBuffer12 = (const DepthBuffer*)depthBuffer;
	assert(depthBuffer12 != nullptr);

	assert_msg(false, "Depth UAVs not yet supported");
}


void DescriptorSet::SetUAV(uint32_t slot, const IGpuBuffer* gpuBuffer)
{
	// TODO: Try this with GetPlatformObject()

	const GpuBuffer* gpuBuffer12 = (const GpuBuffer*)gpuBuffer;
	assert(gpuBuffer12 != nullptr);

	if (m_isRootBuffer)
	{
		assert(slot == 0);
		m_gpuAddress = gpuBuffer12->GetGpuAddress();
	}
	else
	{
		auto descriptor = gpuBuffer12->GetUavHandle();
		SetDescriptor(slot, descriptor);
	}
}


void DescriptorSet::SetCBV(uint32_t slot, const IGpuBuffer* gpuBuffer)
{
	// TODO: Try this with GetPlatformObject()

	const GpuBuffer* gpuBuffer12 = (const GpuBuffer*)gpuBuffer;
	assert(gpuBuffer12 != nullptr);

	if (m_isRootBuffer)
	{
		assert(slot == 0);
		m_gpuAddress = gpuBuffer12->GetGpuAddress();
	}
	else
	{
		auto descriptor = gpuBuffer12->GetCbvHandle();
		SetDescriptor(slot, descriptor);
	}
}


void DescriptorSet::SetDynamicOffset(uint32_t offset)
{
	m_dynamicOffset = offset;
}


void DescriptorSet::UpdateGpuDescriptors()
{
	if (m_dirtyBits == 0 || m_numDescriptors == 0)
		return;

	const D3D12_DESCRIPTOR_HEAP_TYPE heapType = m_isSamplerTable
		? D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
		: D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	ID3D12Device* d3d12Device = m_device->GetD3D12Device();

	uint32_t descriptorSize = d3d12Device->GetDescriptorHandleIncrementSize(heapType);

	unsigned long setBit{ 0 };
	uint32_t paramIndex{ 0 };
	while (_BitScanForward(&setBit, m_dirtyBits))
	{
		DescriptorHandle offsetHandle = m_descriptorHandle + paramIndex * descriptorSize;
		m_dirtyBits &= ~(1 << setBit);

		d3d12Device->CopyDescriptorsSimple(1, offsetHandle.GetCpuHandle(), m_descriptors[paramIndex], heapType);

		++paramIndex;
	}

	assert(m_dirtyBits == 0);
}


bool DescriptorSet::HasBindableDescriptors() const
{
	return m_isRootBuffer || !m_descriptors.empty();
}


D3D12_GPU_DESCRIPTOR_HANDLE DescriptorSet::GetGpuDescriptorHandle() const
{
	return m_descriptorHandle.GetGpuHandle();
}


uint64_t DescriptorSet::GetGpuAddress() const
{
	return m_gpuAddress;
}


uint64_t DescriptorSet::GetDynamicOffset() const
{
	return m_dynamicOffset;
}


uint64_t DescriptorSet::GetGpuAddressWithOffset() const
{
	return m_gpuAddress + m_dynamicOffset;
}


void DescriptorSet::SetDescriptor(uint32_t slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
	assert(slot < (uint32_t)m_numDescriptors);
	if (m_descriptors[slot].ptr != descriptor.ptr)
	{
		m_descriptors[slot] = descriptor;
		m_dirtyBits |= (1 << slot);
	}
}

} // namespace Luna::DX12