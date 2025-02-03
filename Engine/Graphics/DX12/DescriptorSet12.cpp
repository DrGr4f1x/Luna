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
#include "Graphics\DX12\DescriptorAllocator12.h"
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


DescriptorSet::DescriptorSet()
{
	for (uint32_t j = 0; j < MaxDescriptors; ++j)
	{
		m_descriptors[j].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}
	m_gpuDescriptor.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
}


void DescriptorSet::SetSRV(int paramIndex, const IColorBuffer* colorBuffer)
{
	SetDescriptor(paramIndex, GetSRV(colorBuffer));
}


void DescriptorSet::SetSRV(int paramIndex, const IDepthBuffer* depthBuffer, bool depthSrv)
{
	SetDescriptor(paramIndex, GetSRV(depthBuffer, depthSrv));
}


void DescriptorSet::SetSRV(int paramIndex, const IGpuBuffer* gpuBuffer)
{
	SetDescriptor(paramIndex, GetSRV(gpuBuffer));
}


void DescriptorSet::SetUAV(int paramIndex, const IColorBuffer* colorBuffer, uint32_t uavIndex)
{
	SetDescriptor(paramIndex, GetUAV(colorBuffer, uavIndex));
}


void DescriptorSet::SetUAV(int paramIndex, const IDepthBuffer* colorBuffer)
{
	assert_msg(false, "Depth UAVs not yet supported");
}


void DescriptorSet::SetUAV(int paramIndex, const IGpuBuffer* gpuBuffer)
{
	SetDescriptor(paramIndex, GetUAV(gpuBuffer, 0));
}


void DescriptorSet::SetCBV(int paramIndex, const IGpuBuffer* gpuBuffer)
{
	if (m_bIsRootCBV)
	{
		m_gpuAddress = gpuBuffer->GetNativeObject(NativeObjectType::DX12_GpuVirtualAddress).integer;
	}
	else
	{
		SetDescriptor(paramIndex, GetCBV(gpuBuffer));
	}
}


void DescriptorSet::SetDynamicOffset(uint32_t offset)
{
	assert(m_gpuAddress != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

	m_dynamicOffset = offset;
}


void DescriptorSet::Update()
{
	if (!IsDirty() || m_descriptors.empty())
		return;

	auto device = GetD3D12GraphicsDevice()->GetD3D12Device();

	const uint32_t numDescriptors = __popcnt(m_dirtyBits);

	D3D12_DESCRIPTOR_HEAP_TYPE heapType{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV };

	if (m_bIsSamplerTable)
	{
		heapType = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	}

	DescriptorHandle descHandle = AllocateUserDescriptor(heapType, numDescriptors);
	uint32_t descriptorSize = device->GetDescriptorHandleIncrementSize(heapType);

	m_gpuDescriptor = descHandle.GetGpuHandle();

	unsigned long setBit{ 0 };
	uint32_t index{ 0 };
	while (_BitScanForward(&setBit, m_dirtyBits))
	{
		DescriptorHandle offsetHandle = descHandle + index * descriptorSize;
		m_dirtyBits &= ~(1 << setBit);

		device->CopyDescriptorsSimple(1, offsetHandle.GetCpuHandle(), m_descriptors[index], heapType);

		++index;
	}

	assert(m_dirtyBits == 0);
}


void DescriptorSet::SetDescriptor(int paramIndex, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
	if (m_descriptors[paramIndex].ptr != descriptor.ptr)
	{
		m_descriptors[paramIndex] = descriptor;
		m_dirtyBits |= (1 << paramIndex);
	}
}

} // namespace Luna::DX12