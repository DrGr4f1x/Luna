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

#include "DescriptorSetFactory12.h"

#include "ResourceManager12.h"


namespace Luna::DX12
{

DescriptorSetFactory::DescriptorSetFactory(IResourceManager* owner, ID3D12Device* device)
	: m_owner{ owner }
	, m_device{ device }
{
	for (uint32_t i = 0; i < MaxResources; ++i)
	{
		m_freeList.push(i);
	}

	ClearDescs();
	ClearData();
}


ResourceHandle DescriptorSetFactory::CreateDescriptorSet(const DescriptorSetDesc& descriptorSetDesc)
{
	std::lock_guard lock(m_mutex);

	// Get a descriptor set index allocation
	assert(!m_freeList.empty());
	uint32_t descriptorSetIndex = m_freeList.front();
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

	m_descs[descriptorSetIndex] = descriptorSetDesc;
	m_data[descriptorSetIndex] = data;

	return make_shared<ResourceHandleType>(descriptorSetIndex, IResourceManager::ManagedDescriptorSet, m_owner);
}


void DescriptorSetFactory::Destroy(uint32_t index)
{
	m_freeList.push(index);
	ResetDesc(index);
	ResetData(index);
}


void DescriptorSetFactory::SetSRV(uint32_t index, int slot, const ColorBuffer& colorBuffer)
{
	auto colorBufferHandle = colorBuffer.GetHandle();

	auto descriptor = GetD3D12ResourceManager()->GetSRV(colorBufferHandle.get(), true);
	SetDescriptor(m_data[index], slot, descriptor);
}


void DescriptorSetFactory::SetSRV(uint32_t index, int slot, const DepthBuffer& depthBuffer, bool depthSrv)
{
	auto depthBufferHandle = depthBuffer.GetHandle();

	auto descriptor = GetD3D12ResourceManager()->GetSRV(depthBufferHandle.get(), true);
	SetDescriptor(m_data[index], slot, descriptor);
}


void DescriptorSetFactory::SetSRV(uint32_t index, int slot, const GpuBuffer& gpuBuffer)
{
	auto gpuBufferHandle = gpuBuffer.GetHandle();

	if (m_data[index].isRootBuffer)
	{
		assert(slot == 0);
		uint64_t gpuAddress = GetD3D12ResourceManager()->GetGpuAddress(gpuBufferHandle.get());
		m_data[index].gpuAddress = gpuAddress;
	}
	else
	{
		auto descriptor = GetD3D12ResourceManager()->GetSRV(gpuBufferHandle.get(), true);
		SetDescriptor(m_data[index], slot, descriptor);
	}
}


void DescriptorSetFactory::SetUAV(uint32_t index, int slot, const ColorBuffer& colorBuffer, uint32_t uavIndex)
{
	auto colorBufferHandle = colorBuffer.GetHandle();

	auto descriptor = GetD3D12ResourceManager()->GetUAV(colorBufferHandle.get(), uavIndex);
	SetDescriptor(m_data[index], slot, descriptor);
}


void DescriptorSetFactory::SetUAV(uint32_t index, int slot, const DepthBuffer& depthBuffer)
{
	assert_msg(false, "Depth UAVs not yet supported");
}


void DescriptorSetFactory::SetUAV(uint32_t index, int slot, const GpuBuffer& gpuBuffer)
{
	auto gpuBufferHandle = gpuBuffer.GetHandle();

	if (m_data[index].isRootBuffer)
	{
		assert(slot == 0);
		uint64_t gpuAddress = GetD3D12ResourceManager()->GetGpuAddress(gpuBufferHandle.get());
		m_data[index].gpuAddress = gpuAddress;
	}
	else
	{
		auto descriptor = GetD3D12ResourceManager()->GetUAV(gpuBufferHandle.get());
		SetDescriptor(m_data[index], slot, descriptor);
	}
}


void DescriptorSetFactory::SetCBV(uint32_t index, int slot, const GpuBuffer& gpuBuffer)
{
	auto gpuBufferHandle = gpuBuffer.GetHandle();

	if (m_data[index].isRootBuffer)
	{
		assert(slot == 0);
		uint64_t gpuAddress = GetD3D12ResourceManager()->GetGpuAddress(gpuBufferHandle.get());
		m_data[index].gpuAddress = gpuAddress;
	}
	else
	{
		auto descriptor = GetD3D12ResourceManager()->GetCBV(gpuBufferHandle.get());
		SetDescriptor(m_data[index], slot, descriptor);
	}
}


void DescriptorSetFactory::SetDynamicOffset(uint32_t index, uint32_t offset)
{
	m_data[index].dynamicOffset = offset;
}


void DescriptorSetFactory::UpdateGpuDescriptors(uint32_t index)
{
	auto& data = m_data[index];

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


bool DescriptorSetFactory::HasBindableDescriptors(uint32_t index) const
{
	const auto& data = m_data[index];

	return data.isRootBuffer || !data.descriptors.empty();
}


D3D12_GPU_DESCRIPTOR_HANDLE DescriptorSetFactory::GetGpuDescriptorHandle(uint32_t index) const
{
	return m_data[index].descriptorHandle.GetGpuHandle();
}


uint64_t DescriptorSetFactory::GetDescriptorSetGpuAddress(uint32_t index) const
{
	return m_data[index].gpuAddress;
}


uint64_t DescriptorSetFactory::GetDynamicOffset(uint32_t index) const
{
	return m_data[index].dynamicOffset;
}


uint64_t DescriptorSetFactory::GetGpuAddressWithOffset(uint32_t index) const
{
	return m_data[index].gpuAddress + m_data[index].dynamicOffset;
}


void DescriptorSetFactory::SetDescriptor(DescriptorSetData& data, int slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
	assert(slot >= 0 && slot < (int)data.numDescriptors);
	if (data.descriptors[slot].ptr != descriptor.ptr)
	{
		data.descriptors[slot] = descriptor;
		data.dirtyBits |= (1 << slot);
	}
}

} // namespace Luna::DX12