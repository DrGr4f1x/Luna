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

#include "DescriptorSetPool12.h"

#include "Graphics\ColorBuffer.h"
#include "Graphics\DepthBuffer.h"
#include "Graphics\GpuBuffer.h"


namespace Luna::DX12
{

DescriptorSetPool* g_descriptorSetPool{ nullptr };


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


DescriptorSetPool::DescriptorSetPool(ID3D12Device* device)
	: m_device{ device }
{
	assert(g_descriptorSetPool == nullptr);

	// Populate free list and data arrays
	for (uint32_t i = 0; i < MaxItems; ++i)
	{
		m_freeList.push(i);
		m_descriptorData[i] = DescriptorSetData{};
	}

	g_descriptorSetPool = this;
}


DescriptorSetPool::~DescriptorSetPool()
{
	g_descriptorSetPool = nullptr;
}


DescriptorSetHandle DescriptorSetPool::CreateDescriptorSet(const DescriptorSetDesc& descriptorSetDesc)
{
	std::lock_guard guard(m_allocationMutex);

	assert(!m_freeList.empty());

	uint32_t index = m_freeList.front();
	m_freeList.pop();

	DescriptorSetData data{
		.descriptorHandle	= descriptorSetDesc.descriptorHandle,
		.numDescriptors		= descriptorSetDesc.numDescriptors,
		.isSamplerTable		= descriptorSetDesc.isSamplerTable,
		.isRootBuffer		= descriptorSetDesc.isRootBuffer
	};

	assert(data.numDescriptors <= MaxDescriptorsPerTable);

	for (uint32_t j = 0; j < MaxDescriptorsPerTable; ++j)
	{
		data.descriptors[j].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	m_descriptorData[index] = data;

	return Create<DescriptorSetHandleType>(index, this);
}


void DescriptorSetPool::DestroyHandle(DescriptorSetHandleType* handle)
{
	assert(handle != nullptr);

	std::lock_guard guard(m_allocationMutex);

	// TODO: queue this up to execute in one big batch, e.g. at the end of the frame

	uint32_t index = handle->GetIndex();
	m_descriptorData[index] = DescriptorSetData{};

	m_freeList.push(index);
}


void DescriptorSetPool::SetSRV(DescriptorSetHandleType* handle, int slot, const IColorBuffer* colorBuffer)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	SetDescriptor(data, slot, GetSRV(colorBuffer));
}


void DescriptorSetPool::SetSRV(DescriptorSetHandleType* handle, int slot, const IDepthBuffer* depthBuffer, bool depthSrv)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	SetDescriptor(data, slot, GetSRV(depthBuffer, depthSrv));
}


void DescriptorSetPool::SetSRV(DescriptorSetHandleType* handle, int slot, const IGpuBuffer* gpuBuffer)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	if (data.isRootBuffer)
	{
		assert(slot == 0);
		data.gpuAddress = gpuBuffer->GetNativeObject(NativeObjectType::DX12_GpuVirtualAddress).integer;
	}
	else
	{
		SetDescriptor(data, slot, GetSRV(gpuBuffer));
	}
}


void DescriptorSetPool::SetUAV(DescriptorSetHandleType* handle, int slot, const IColorBuffer* colorBuffer, uint32_t uavIndex)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	SetDescriptor(data, slot, GetUAV(colorBuffer, uavIndex));
}


void DescriptorSetPool::SetUAV(DescriptorSetHandleType* handle, int slot, const IDepthBuffer* colorBuffer)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	assert_msg(false, "Depth UAVs not yet supported");
}


void DescriptorSetPool::SetUAV(DescriptorSetHandleType* handle, int slot, const IGpuBuffer* gpuBuffer)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	if (data.isRootBuffer)
	{
		assert(slot == 0);
		data.gpuAddress = gpuBuffer->GetNativeObject(NativeObjectType::DX12_GpuVirtualAddress).integer;
	}
	else
	{
		SetDescriptor(data, slot, GetUAV(gpuBuffer, 0));
	}
}


void DescriptorSetPool::SetCBV(DescriptorSetHandleType* handle, int slot, const IGpuBuffer* gpuBuffer)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	if (data.isRootBuffer)
	{
		assert(slot == 0);
		data.gpuAddress = gpuBuffer->GetNativeObject(NativeObjectType::DX12_GpuVirtualAddress).integer;
	}
	else
	{
		SetDescriptor(data, slot, GetCBV(gpuBuffer));
	}
}


void DescriptorSetPool::SetDynamicOffset(DescriptorSetHandleType* handle, uint32_t offset)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	assert(data.gpuAddress != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

	data.dynamicOffset = offset;
}


void DescriptorSetPool::UpdateGpuDescriptors(DescriptorSetHandleType* handle)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	if (data.dirtyBits == 0 || data.numDescriptors == 0)
		return;

	const D3D12_DESCRIPTOR_HEAP_TYPE heapType = data.isSamplerTable
		? D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
		: D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	uint32_t descriptorSize = m_device->GetDescriptorHandleIncrementSize(heapType);

	unsigned long setBit{ 0 };
	uint32_t paramIndex{ 0 };
	while (_BitScanForward(&setBit, data.dirtyBits))
	{
		DescriptorHandle offsetHandle = data.descriptorHandle + paramIndex * descriptorSize;
		data.dirtyBits &= ~(1 << setBit);

		m_device->CopyDescriptorsSimple(1, offsetHandle.GetCpuHandle(), data.descriptors[paramIndex], heapType);

		++paramIndex;
	}

	assert(data.dirtyBits == 0);
}


void DescriptorSetPool::SetDescriptor(DescriptorSetData& data, int slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
	assert(slot >= 0 && slot < (int)data.numDescriptors);
	if (data.descriptors[slot].ptr != descriptor.ptr)
	{
		data.descriptors[slot] = descriptor;
		data.dirtyBits |= (1 << slot);
	}
}


bool DescriptorSetPool::HasBindableDescriptors(DescriptorSetHandleType* handle) const
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	return data.isRootBuffer || !data.descriptors.empty();
}


D3D12_GPU_DESCRIPTOR_HANDLE DescriptorSetPool::GetGpuDescriptorHandle(DescriptorSetHandleType* handle) const
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	return data.descriptorHandle.GetGpuHandle();
}


uint64_t DescriptorSetPool::GetGpuAddress(DescriptorSetHandleType* handle) const
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	return data.gpuAddress;
}


uint64_t DescriptorSetPool::GetDynamicOffset(DescriptorSetHandleType* handle) const
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	return data.dynamicOffset;
}


uint64_t DescriptorSetPool::GetGpuAddressWithOffset(DescriptorSetHandleType* handle) const
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	return data.gpuAddress + data.dynamicOffset;
}


DescriptorSetPool* const GetD3D12DescriptorSetPool()
{
	return g_descriptorSetPool;
}

} // namespace Luna::DX12