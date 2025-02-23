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

#include "Graphics\DX12\ColorBufferManager12.h"
#include "Graphics\DX12\DepthBufferManager12.h"
#include "Graphics\DX12\GpuBufferManager12.h"


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


inline D3D12_CPU_DESCRIPTOR_HANDLE GetUAV(const IGpuResource* gpuResource, uint32_t uavIndex)
{
	D3D12_CPU_DESCRIPTOR_HANDLE uavHandle{ .ptr = gpuResource->GetNativeObject(NativeObjectType::DX12_UAV, uavIndex).integer };
	ValidateDescriptor(uavHandle);
	return uavHandle;
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


void DescriptorSetPool::SetSRV(DescriptorSetHandleType* handle, int slot, const ColorBuffer& colorBuffer)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	auto colorBufferManager = GetD3D12ColorBufferManager();
	auto colorBufferHandle = colorBuffer.GetHandle();

	SetDescriptor(data, slot, colorBufferManager->GetSRV(colorBufferHandle.get()));
}


void DescriptorSetPool::SetSRV(DescriptorSetHandleType* handle, int slot, const DepthBuffer& depthBuffer, bool depthSrv)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	auto depthBufferManager = GetD3D12DepthBufferManager();
	auto depthBufferHandle = depthBuffer.GetHandle();

	SetDescriptor(data, slot, depthBufferManager->GetSRV(depthBufferHandle.get(), depthSrv));
}


void DescriptorSetPool::SetSRV(DescriptorSetHandleType* handle, int slot, const GpuBuffer& gpuBuffer)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	auto gpuBufferManager = GetD3D12GpuBufferManager();
	GpuBufferHandle gpuBufferHandle = gpuBuffer.GetHandle();

	if (data.isRootBuffer)
	{
		assert(slot == 0);
		data.gpuAddress = gpuBufferManager->GetGpuAddress(gpuBufferHandle.get());
	}
	else
	{
		SetDescriptor(data, slot, gpuBufferManager->GetSRV(gpuBufferHandle.get()));
	}
}


void DescriptorSetPool::SetUAV(DescriptorSetHandleType* handle, int slot, const ColorBuffer& colorBuffer, uint32_t uavIndex)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	auto colorBufferManager = GetD3D12ColorBufferManager();
	auto colorBufferHandle = colorBuffer.GetHandle();

	SetDescriptor(data, slot, colorBufferManager->GetUAV(colorBufferHandle.get(), uavIndex));
}


void DescriptorSetPool::SetUAV(DescriptorSetHandleType* handle, int slot, const DepthBuffer& depthBuffer)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	auto depthBufferManager = GetD3D12DepthBufferManager();
	auto depthBufferHandle = depthBuffer.GetHandle();

	assert_msg(false, "Depth UAVs not yet supported");
}


void DescriptorSetPool::SetUAV(DescriptorSetHandleType* handle, int slot, const GpuBuffer& gpuBuffer)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	auto gpuBufferManager = GetD3D12GpuBufferManager();
	GpuBufferHandle gpuBufferHandle = gpuBuffer.GetHandle();

	if (data.isRootBuffer)
	{
		assert(slot == 0);
		data.gpuAddress = gpuBufferManager->GetGpuAddress(gpuBufferHandle.get());
	}
	else
	{
		SetDescriptor(data, slot, gpuBufferManager->GetUAV(gpuBufferHandle.get()));
	}
}


void DescriptorSetPool::SetCBV(DescriptorSetHandleType* handle, int slot, const GpuBuffer& gpuBuffer)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	auto gpuBufferManager = GetD3D12GpuBufferManager();
	GpuBufferHandle gpuBufferHandle = gpuBuffer.GetHandle();

	if (data.isRootBuffer)
	{
		assert(slot == 0);
		data.gpuAddress = gpuBufferManager->GetGpuAddress(gpuBufferHandle.get());
	}
	else
	{
		SetDescriptor(data, slot, gpuBufferManager->GetCBV(gpuBufferHandle.get()));
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