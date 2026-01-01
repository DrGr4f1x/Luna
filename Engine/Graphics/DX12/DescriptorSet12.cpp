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


void DescriptorSet::SetBindlessSRVs(uint32_t srvRegister, std::span<const IDescriptor*> descriptors)
{
	assert(!m_isSamplerTable);
	assert(m_rootParameter.parameterType == RootParameterType::Table);

	// TODO: Add some safety checks here

	const uint32_t srvOffset = GetSrvOffset(srvRegister);

	for (size_t i = 0; i < descriptors.size(); ++i)
	{
		const Descriptor* descriptor = (const Descriptor*)descriptors[i];

		const uint32_t descriptorSlot = srvOffset + (uint32_t)i;

		UpdateDescriptor(descriptorSlot, descriptor->GetHandleCPU());
	}
}


void DescriptorSet::SetSRV(uint32_t srvRegister, ColorBufferPtr colorBuffer)
{
	const uint32_t descriptorSlot = GetSrvOffset(srvRegister);

	UpdateDescriptor(descriptorSlot, ((const Descriptor*)colorBuffer->GetSrvDescriptor())->GetHandleCPU());
}


void DescriptorSet::SetSRV(uint32_t srvRegister, DepthBufferPtr depthBuffer, bool depthSrv)
{
	const uint32_t descriptorSlot = GetSrvOffset(srvRegister);

	UpdateDescriptor(descriptorSlot, ((const Descriptor*)depthBuffer->GetSrvDescriptor(depthSrv))->GetHandleCPU());
}


void DescriptorSet::SetSRV(uint32_t srvRegister, GpuBufferPtr gpuBuffer)
{
	const GpuBuffer* gpuBuffer12 = (const GpuBuffer*)gpuBuffer.get();
	assert(gpuBuffer12 != nullptr);

	auto cpuHandle = ((const Descriptor*)gpuBuffer->GetSrvDescriptor())->GetHandleCPU();

	const uint32_t descriptorSlot = GetSrvOffset(srvRegister);

	UpdateDescriptor(descriptorSlot, cpuHandle);
}


void DescriptorSet::SetSRV(uint32_t srvRegister, TexturePtr texture)
{
	const uint32_t descriptorSlot = GetSrvOffset(srvRegister);

	UpdateDescriptor(descriptorSlot, ((const Descriptor*)texture->GetDescriptor())->GetHandleCPU());
}


void DescriptorSet::SetUAV(uint32_t uavRegister, ColorBufferPtr colorBuffer, uint32_t uavIndex)
{
	const uint32_t descriptorSlot = GetUavOffset(uavRegister);

	UpdateDescriptor(descriptorSlot, ((const Descriptor*)colorBuffer->GetUavDescriptor(uavIndex))->GetHandleCPU());
}


void DescriptorSet::SetUAV(uint32_t uavRegister, DepthBufferPtr depthBuffer)
{
	assert_msg(false, "Depth UAVs not yet supported");

	const uint32_t descriptorSlot = GetUavOffset(uavRegister);
}


void DescriptorSet::SetUAV(uint32_t uavRegister, GpuBufferPtr gpuBuffer)
{
	const GpuBuffer* gpuBuffer12 = (const GpuBuffer*)gpuBuffer.get();
	assert(gpuBuffer12 != nullptr);

	auto cpuHandle = ((const Descriptor*)gpuBuffer->GetUavDescriptor())->GetHandleCPU();

	const uint32_t descriptorSlot = GetUavOffset(uavRegister);

	UpdateDescriptor(descriptorSlot, cpuHandle);
}


void DescriptorSet::SetCBV(uint32_t cbvRegister, GpuBufferPtr gpuBuffer)
{
	const GpuBuffer* gpuBuffer12 = (const GpuBuffer*)gpuBuffer.get();
	assert(gpuBuffer12 != nullptr);

	auto cpuHandle = ((const Descriptor*)gpuBuffer12->GetCbvDescriptor())->GetHandleCPU();

	const uint32_t descriptorSlot = GetCbvOffset(cbvRegister);

	UpdateDescriptor(descriptorSlot, cpuHandle);
}


void DescriptorSet::SetSampler(uint32_t samplerRegister, SamplerPtr sampler)
{
	const uint32_t descriptorSlot = GetSamplerOffset(samplerRegister);

	UpdateDescriptor(descriptorSlot, ((const Descriptor*)sampler->GetDescriptor())->GetHandleCPU());
}


bool DescriptorSet::HasBindableDescriptors() const
{
	return !m_descriptors.empty();
}


D3D12_GPU_DESCRIPTOR_HANDLE DescriptorSet::GetGpuDescriptorHandle() const
{
	return m_descriptorHandle.GetGpuHandle();
}


uint64_t DescriptorSet::GetGpuAddress() const
{
	return m_gpuAddress;
}


uint64_t DescriptorSet::GetGpuAddressWithOffset() const
{
	return m_gpuAddress;
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


uint32_t DescriptorSet::GetSrvOffset(uint32_t srvRegister) const
{
	auto ret = m_srvOffsets.find(srvRegister);
	assert(ret != m_srvOffsets.end());
	return ret->second;
}


uint32_t DescriptorSet::GetCbvOffset(uint32_t cbvRegister) const
{
	auto ret = m_cbvOffsets.find(cbvRegister);
	assert(ret != m_cbvOffsets.end());
	return ret->second;
}


uint32_t DescriptorSet::GetUavOffset(uint32_t uavRegister) const
{
	auto ret = m_uavOffsets.find(uavRegister);
	assert(ret != m_uavOffsets.end());
	return ret->second;
}

} // namespace Luna::DX12