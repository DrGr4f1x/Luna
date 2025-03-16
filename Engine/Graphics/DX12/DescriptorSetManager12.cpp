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

#include "DescriptorSetManager12.h"

#include "ResourceManager12.h"


namespace Luna::DX12
{

DescriptorSetManager* g_descriptorSetManager{ nullptr };


inline void ValidateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	assert(handle.ptr != 0);
}


DescriptorSetManager::DescriptorSetManager(ID3D12Device* device)
	: m_device{ device }
{
	assert(g_descriptorSetManager == nullptr);

	// Populate free list and data arrays
	for (uint32_t i = 0; i < MaxItems; ++i)
	{
		m_freeList.push(i);
		m_descriptorData[i] = DescriptorSetData{};
	}

	g_descriptorSetManager = this;
}


DescriptorSetManager::~DescriptorSetManager()
{
	g_descriptorSetManager = nullptr;
}


DescriptorSetHandle DescriptorSetManager::CreateDescriptorSet(const DescriptorSetDesc& descriptorSetDesc)
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


void DescriptorSetManager::DestroyHandle(DescriptorSetHandleType* handle)
{
	assert(handle != nullptr);

	std::lock_guard guard(m_allocationMutex);

	// TODO: queue this up to execute in one big batch, e.g. at the end of the frame

	uint32_t index = handle->GetIndex();
	m_descriptorData[index] = DescriptorSetData{};

	m_freeList.push(index);
}


void DescriptorSetManager::SetSRV(DescriptorSetHandleType* handle, int slot, const ColorBuffer& colorBuffer)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	auto resourceManager = GetD3D12ResourceManager();
	auto colorBufferHandle = colorBuffer.GetHandle();

	SetDescriptor(data, slot, resourceManager->GetSRV(colorBufferHandle.get(), true));
}


void DescriptorSetManager::SetSRV(DescriptorSetHandleType* handle, int slot, const DepthBuffer& depthBuffer, bool depthSrv)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	auto resourceManager = GetD3D12ResourceManager();
	auto depthBufferHandle = depthBuffer.GetHandle();

	SetDescriptor(data, slot, resourceManager->GetSRV(depthBufferHandle.get(), depthSrv));
}


void DescriptorSetManager::SetSRV(DescriptorSetHandleType* handle, int slot, const GpuBuffer& gpuBuffer)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	auto resourceManager = GetD3D12ResourceManager();
	auto gpuBufferHandle = gpuBuffer.GetHandle();

	if (data.isRootBuffer)
	{
		assert(slot == 0);
		data.gpuAddress = resourceManager->GetGpuAddress(gpuBufferHandle.get());
	}
	else
	{
		SetDescriptor(data, slot, resourceManager->GetSRV(gpuBufferHandle.get(), true));
	}
}


void DescriptorSetManager::SetUAV(DescriptorSetHandleType* handle, int slot, const ColorBuffer& colorBuffer, uint32_t uavIndex)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	auto resourceManager = GetD3D12ResourceManager();
	auto colorBufferHandle = colorBuffer.GetHandle();

	SetDescriptor(data, slot, resourceManager->GetUAV(colorBufferHandle.get(), uavIndex));
}


void DescriptorSetManager::SetUAV(DescriptorSetHandleType* handle, int slot, const DepthBuffer& depthBuffer)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	auto resourceManager = GetD3D12ResourceManager();
	auto depthBufferHandle = depthBuffer.GetHandle();

	assert_msg(false, "Depth UAVs not yet supported");
}


void DescriptorSetManager::SetUAV(DescriptorSetHandleType* handle, int slot, const GpuBuffer& gpuBuffer)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	auto resourceManager = GetD3D12ResourceManager();
	auto gpuBufferHandle = gpuBuffer.GetHandle();

	if (data.isRootBuffer)
	{
		assert(slot == 0);
		data.gpuAddress = resourceManager->GetGpuAddress(gpuBufferHandle.get());
	}
	else
	{
		SetDescriptor(data, slot, resourceManager->GetUAV(gpuBufferHandle.get()));
	}
}


void DescriptorSetManager::SetCBV(DescriptorSetHandleType* handle, int slot, const GpuBuffer& gpuBuffer)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	auto resourceManager = GetD3D12ResourceManager();
	auto gpuBufferHandle = gpuBuffer.GetHandle();

	if (data.isRootBuffer)
	{
		assert(slot == 0);
		data.gpuAddress = resourceManager->GetGpuAddress(gpuBufferHandle.get());
	}
	else
	{
		SetDescriptor(data, slot, resourceManager->GetCBV(gpuBufferHandle.get()));
	}
}


void DescriptorSetManager::SetDynamicOffset(DescriptorSetHandleType* handle, uint32_t offset)
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	assert(data.gpuAddress != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

	data.dynamicOffset = offset;
}


void DescriptorSetManager::UpdateGpuDescriptors(DescriptorSetHandleType* handle)
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


void DescriptorSetManager::SetDescriptor(DescriptorSetData& data, int slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
	assert(slot >= 0 && slot < (int)data.numDescriptors);
	if (data.descriptors[slot].ptr != descriptor.ptr)
	{
		data.descriptors[slot] = descriptor;
		data.dirtyBits |= (1 << slot);
	}
}


bool DescriptorSetManager::HasBindableDescriptors(DescriptorSetHandleType* handle) const
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	return data.isRootBuffer || !data.descriptors.empty();
}


D3D12_GPU_DESCRIPTOR_HANDLE DescriptorSetManager::GetGpuDescriptorHandle(DescriptorSetHandleType* handle) const
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	return data.descriptorHandle.GetGpuHandle();
}


uint64_t DescriptorSetManager::GetGpuAddress(DescriptorSetHandleType* handle) const
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	return data.gpuAddress;
}


uint64_t DescriptorSetManager::GetDynamicOffset(DescriptorSetHandleType* handle) const
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	return data.dynamicOffset;
}


uint64_t DescriptorSetManager::GetGpuAddressWithOffset(DescriptorSetHandleType* handle) const
{
	assert(handle != 0);

	uint32_t index = handle->GetIndex();
	auto& data = m_descriptorData[index];

	return data.gpuAddress + data.dynamicOffset;
}


DescriptorSetManager* const GetD3D12DescriptorSetManager()
{
	return g_descriptorSetManager;
}

} // namespace Luna::DX12