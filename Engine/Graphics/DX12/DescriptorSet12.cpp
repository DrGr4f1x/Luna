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
#include "Descriptor12.h"
#include "Device12.h"
#include "GpuBuffer12.h"
#include "Sampler12.h"
#include "Texture12.h"


namespace Luna::DX12
{

DescriptorSet::DescriptorSet(Device* device, const RootParameter& rootParameter)
	: m_device{ device }
	, m_rootParameter{ rootParameter }
{}


void DescriptorSet::SetSRV(uint32_t slot, const IDescriptor* descriptor)
{
	const Descriptor* descriptor12 = (const Descriptor*)descriptor;

	assert(!m_isSamplerTable);
	assert(IsDescriptorTypeSRV(descriptor12->GetDescriptorType()));
	assert(m_rootParameter.parameterType == RootParameterType::RootSRV || m_rootParameter.parameterType == RootParameterType::Table);
	if (m_isRootBuffer)
	{
		m_gpuAddress = descriptor12->GetGpuAddress();
	}
	else if (m_rootParameter.parameterType == RootParameterType::Table)
	{
		assert(m_rootParameter.GetDescriptorType(slot) == descriptor12->GetDescriptorType());
		UpdateDescriptor(slot, descriptor12->GetHandleCPU());
	}	
}


void DescriptorSet::SetUAV(uint32_t slot, const IDescriptor* descriptor)
{
	const Descriptor* descriptor12 = (const Descriptor*)descriptor;

	assert(!m_isSamplerTable);
	assert(IsDescriptorTypeUAV(descriptor12->GetDescriptorType()));
	assert(m_rootParameter.parameterType == RootParameterType::RootUAV || m_rootParameter.parameterType == RootParameterType::Table);
	if (m_isRootBuffer)
	{
		m_gpuAddress = descriptor12->GetGpuAddress();
	}
	else if (m_rootParameter.parameterType == RootParameterType::Table)
	{
		assert(m_rootParameter.GetDescriptorType(slot) == descriptor12->GetDescriptorType());
		UpdateDescriptor(slot, descriptor12->GetHandleCPU());
	}
}


void DescriptorSet::SetCBV(uint32_t slot, const IDescriptor* descriptor)
{
	const Descriptor* descriptor12 = (const Descriptor*)descriptor;

	assert(!m_isSamplerTable);
	assert(descriptor12->GetDescriptorType() == DescriptorType::ConstantBuffer);
	assert(m_rootParameter.parameterType == RootParameterType::RootCBV || m_rootParameter.parameterType == RootParameterType::Table);
	if (m_isRootBuffer)
	{
		m_gpuAddress = descriptor12->GetGpuAddress();
	}
	else if (m_rootParameter.parameterType == RootParameterType::Table)
	{
		assert(m_rootParameter.GetDescriptorType(slot) == descriptor12->GetDescriptorType());
		UpdateDescriptor(slot, descriptor12->GetHandleCPU());
	}
}


void DescriptorSet::SetSampler(uint32_t slot, const IDescriptor* descriptor)
{
	const Descriptor* descriptor12 = (const Descriptor*)descriptor;

	assert(m_isSamplerTable);
	assert(descriptor12->GetDescriptorType() == DescriptorType::Sampler);
	assert(m_rootParameter.parameterType == RootParameterType::Table);
	assert(m_rootParameter.GetDescriptorType(slot) == descriptor12->GetDescriptorType());
	
	UpdateDescriptor(slot, descriptor12->GetHandleCPU());
}


void DescriptorSet::SetSRV(uint32_t slot, ColorBufferPtr colorBuffer)
{
	SetDescriptor(slot,((const Descriptor*)colorBuffer->GetSrvDescriptor())->GetHandleCPU());
}


void DescriptorSet::SetSRV(uint32_t slot, DepthBufferPtr depthBuffer, bool depthSrv)
{
	SetDescriptor(slot, ((const Descriptor*)depthBuffer->GetSrvDescriptor(depthSrv))->GetHandleCPU());
}


void DescriptorSet::SetSRV(uint32_t slot, GpuBufferPtr gpuBuffer)
{
	// TODO: Try this with GetPlatformObject()

	const GpuBuffer* gpuBuffer12 = (const GpuBuffer*)gpuBuffer.get();
	assert(gpuBuffer12 != nullptr);

	if (m_isRootBuffer)
	{
		m_gpuAddress = gpuBuffer12->GetGpuAddress();
	}
	else
	{
		auto cpuHandle = ((const Descriptor*)gpuBuffer12->GetSrvDescriptor())->GetHandleCPU();
		SetDescriptor(slot, cpuHandle);
	}
}


void DescriptorSet::SetSRV(uint32_t slot, TexturePtr texture)
{
	// TODO: Try this with GetPlatformObject()

	const Texture* texture12 = (const Texture*)texture.Get();
	assert(texture12 != nullptr);

	const auto& descriptor = texture12->GetSrvDescriptor();
	SetDescriptor(slot, descriptor.GetHandleCPU());
}


void DescriptorSet::SetUAV(uint32_t slot, ColorBufferPtr colorBuffer, uint32_t uavIndex)
{
	SetDescriptor(slot, ((const Descriptor*)colorBuffer->GetUavDescriptor(uavIndex))->GetHandleCPU());
}


void DescriptorSet::SetUAV(uint32_t slot, DepthBufferPtr depthBuffer)
{
	// TODO: Try this with GetPlatformObject()

	const DepthBuffer* depthBuffer12 = (const DepthBuffer*)depthBuffer.get();
	assert(depthBuffer12 != nullptr);

	assert_msg(false, "Depth UAVs not yet supported");
}


void DescriptorSet::SetUAV(uint32_t slot, GpuBufferPtr gpuBuffer)
{
	// TODO: Try this with GetPlatformObject()

	const GpuBuffer* gpuBuffer12 = (const GpuBuffer*)gpuBuffer.get();
	assert(gpuBuffer12 != nullptr);

	if (m_isRootBuffer)
	{
		m_gpuAddress = gpuBuffer12->GetGpuAddress();
	}
	else
	{
		auto cpuHandle = ((const Descriptor*)gpuBuffer12->GetUavDescriptor())->GetHandleCPU();
		SetDescriptor(slot, cpuHandle);
	}
}


void DescriptorSet::SetCBV(uint32_t slot, GpuBufferPtr gpuBuffer)
{
	// TODO: Try this with GetPlatformObject()

	const GpuBuffer* gpuBuffer12 = (const GpuBuffer*)gpuBuffer.get();
	assert(gpuBuffer12 != nullptr);

	if (m_isRootBuffer)
	{
		m_gpuAddress = gpuBuffer12->GetGpuAddress();
	}
	else
	{
		auto cpuHandle = ((const Descriptor*)gpuBuffer12->GetCbvDescriptor())->GetHandleCPU();
		SetDescriptor(slot, cpuHandle);
	}
}


void DescriptorSet::SetSampler(uint32_t slot, SamplerPtr sampler)
{
	SetDescriptor(slot, ((const Descriptor*)sampler->GetDescriptor())->GetHandleCPU());
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


void DescriptorSet::UpdateDescriptor(uint32_t slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
	const D3D12_DESCRIPTOR_HEAP_TYPE heapType = m_isSamplerTable
		? D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
		: D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	ID3D12Device* d3d12Device = m_device->GetD3D12Device();

	uint32_t descriptorSize = d3d12Device->GetDescriptorHandleIncrementSize(heapType);

	DescriptorHandle offsetHandle = m_descriptorHandle + slot * descriptorSize;

	d3d12Device->CopyDescriptorsSimple(1, offsetHandle.GetCpuHandle(), descriptor, heapType);
}

} // namespace Luna::DX12