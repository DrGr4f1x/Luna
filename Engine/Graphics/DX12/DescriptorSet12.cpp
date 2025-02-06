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

#include "Graphics\ColorBuffer.h"
#include "Graphics\DepthBuffer.h"
#include "Graphics\GpuBuffer.h"
#include "Graphics\DX12\Device12.h"


namespace Luna::DX12
{

inline void ValidateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	assert(handle.ptr != 0);
}


inline D3D12_CPU_DESCRIPTOR_HANDLE GetSRV(const IGpuResource* gpuResource)
{
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle{ .ptr = gpuResource->GetNativeObject(NativeObjectType::DX12_SRV).integer };
	ValidateDescriptor(srvHandle);
	return srvHandle;
}


inline D3D12_CPU_DESCRIPTOR_HANDLE GetSRV(const IDepthBuffer* depthBuffer, bool depthSrv)
{
	NativeObjectType type = depthSrv ? NativeObjectType::DX12_SRV_Depth : NativeObjectType::DX12_SRV_Stencil;
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle{ .ptr = depthBuffer->GetNativeObject(type).integer };
	ValidateDescriptor(srvHandle);
	return srvHandle;
}


inline D3D12_CPU_DESCRIPTOR_HANDLE GetUAV(const IGpuResource* gpuResource, uint32_t uavIndex)
{
	D3D12_CPU_DESCRIPTOR_HANDLE uavHandle{ .ptr = gpuResource->GetNativeObject(NativeObjectType::DX12_UAV, uavIndex).integer };
	ValidateDescriptor(uavHandle);
	return uavHandle;
}


inline D3D12_CPU_DESCRIPTOR_HANDLE GetCBV(const IGpuBuffer* gpuBuffer)
{
	D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle{ .ptr = gpuBuffer->GetNativeObject(NativeObjectType::DX12_CBV).integer };
	ValidateDescriptor(cbvHandle);
	return cbvHandle;
}


DescriptorSet::DescriptorSet(const DescriptorSetDescExt& descriptorSetDescExt)
	: m_descriptorHandle{ descriptorSetDescExt.descriptorHandle }
	, m_numDescriptors{ descriptorSetDescExt.numDescriptors }
	, m_isSamplerTable{ descriptorSetDescExt.isSamplerTable }
	, m_isRootBuffer{ descriptorSetDescExt.isRootBuffer }
{
	assert(m_numDescriptors <= MaxDescriptorsPerTable);

	for (uint32_t j = 0; j < MaxDescriptorsPerTable; ++j)
	{
		m_descriptors[j].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}
}


void DescriptorSet::SetSRV(int slot, const IColorBuffer* colorBuffer)
{
	SetDescriptor(slot, GetSRV(colorBuffer));
}


void DescriptorSet::SetSRV(int slot, const IDepthBuffer* depthBuffer, bool depthSrv)
{
	SetDescriptor(slot, GetSRV(depthBuffer, depthSrv));
}


void DescriptorSet::SetSRV(int slot, const IGpuBuffer* gpuBuffer)
{
	if (m_isRootBuffer)
	{
		assert(slot == 0);
		m_gpuAddress = gpuBuffer->GetNativeObject(NativeObjectType::DX12_GpuVirtualAddress).integer;
	}
	else
	{
		SetDescriptor(slot, GetSRV(gpuBuffer));
	}
}


void DescriptorSet::SetUAV(int slot, const IColorBuffer* colorBuffer, uint32_t uavIndex)
{
	SetDescriptor(slot, GetUAV(colorBuffer, uavIndex));
}


void DescriptorSet::SetUAV(int slot, const IDepthBuffer* colorBuffer)
{
	assert_msg(false, "Depth UAVs not yet supported");
}


void DescriptorSet::SetUAV(int slot, const IGpuBuffer* gpuBuffer)
{
	if (m_isRootBuffer)
	{
		assert(slot == 0);
		m_gpuAddress = gpuBuffer->GetNativeObject(NativeObjectType::DX12_GpuVirtualAddress).integer;
	}
	else
	{
		SetDescriptor(slot, GetUAV(gpuBuffer, 0));
	}
}


void DescriptorSet::SetCBV(int slot, const IGpuBuffer* gpuBuffer)
{
	if (m_isRootBuffer)
	{
		assert(slot == 0);
		m_gpuAddress = gpuBuffer->GetNativeObject(NativeObjectType::DX12_GpuVirtualAddress).integer;
	}
	else
	{
		SetDescriptor(slot, GetCBV(gpuBuffer));
	}
}


void DescriptorSet::SetDynamicOffset(uint32_t offset)
{
	assert(m_gpuAddress != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

	m_dynamicOffset = offset;
}


void DescriptorSet::Update(ID3D12Device* device)
{
	if (!IsDirty() || m_numDescriptors == 0)
		return;

	const D3D12_DESCRIPTOR_HEAP_TYPE heapType = m_isSamplerTable 
		? D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
		: D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	//DescriptorHandle descHandle = AllocateUserDescriptor(heapType, numDescriptors);
	uint32_t descriptorSize = device->GetDescriptorHandleIncrementSize(heapType);

	unsigned long setBit{ 0 };
	uint32_t index{ 0 };
	while (_BitScanForward(&setBit, m_dirtyBits))
	{
		DescriptorHandle offsetHandle = m_descriptorHandle + index * descriptorSize;
		m_dirtyBits &= ~(1 << setBit);

		device->CopyDescriptorsSimple(1, offsetHandle.GetCpuHandle(), m_descriptors[index], heapType);

		++index;
	}

	assert(m_dirtyBits == 0);
}


void DescriptorSet::SetDescriptor(int slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
	assert(slot >= 0 && slot < (int)m_numDescriptors);
	if (m_descriptors[slot].ptr != descriptor.ptr)
	{
		m_descriptors[slot] = descriptor;
		m_dirtyBits |= (1 << slot);
	}
}

} // namespace Luna::DX12