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
	UpdateDescriptor(slot,((const Descriptor*)colorBuffer->GetSrvDescriptor())->GetHandleCPU());
}


void DescriptorSet::SetSRV(uint32_t slot, DepthBufferPtr depthBuffer, bool depthSrv)
{
	UpdateDescriptor(slot, ((const Descriptor*)depthBuffer->GetSrvDescriptor(depthSrv))->GetHandleCPU());
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
		auto cpuHandle = ((const Descriptor*)gpuBuffer->GetSrvDescriptor())->GetHandleCPU();
		UpdateDescriptor(slot, cpuHandle);
	}
}


void DescriptorSet::SetSRV(uint32_t slot, TexturePtr texture)
{
	UpdateDescriptor(slot, ((const Descriptor*)texture->GetDescriptor())->GetHandleCPU());
}


void DescriptorSet::SetUAV(uint32_t slot, ColorBufferPtr colorBuffer, uint32_t uavIndex)
{
	UpdateDescriptor(slot, ((const Descriptor*)colorBuffer->GetUavDescriptor(uavIndex))->GetHandleCPU());
}


void DescriptorSet::SetUAV(uint32_t slot, DepthBufferPtr depthBuffer)
{
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
		auto cpuHandle = ((const Descriptor*)gpuBuffer->GetUavDescriptor())->GetHandleCPU();
		UpdateDescriptor(slot, cpuHandle);
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
		UpdateDescriptor(slot, cpuHandle);
	}
}


void DescriptorSet::SetSampler(uint32_t slot, SamplerPtr sampler)
{
	UpdateDescriptor(slot, ((const Descriptor*)sampler->GetDescriptor())->GetHandleCPU());
}


void DescriptorSet::SetDynamicOffset(uint32_t offset)
{
	m_dynamicOffset = offset;
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