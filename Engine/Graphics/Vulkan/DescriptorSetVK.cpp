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

#include "DescriptorSetVK.h"

#include "ColorBufferVK.h"
#include "DepthBufferVK.h"
#include "DescriptorVK.h"
#include "DeviceVK.h"
#include "GpuBufferVK.h"
#include "SamplerVK.h"
#include "TextureVK.h"

using namespace std;


namespace Luna::VK
{

BufferDescriptorSet::BufferDescriptorSet(Device* device, const RootParameter& rootParameter)
	: m_device{ device }
	, m_rootParameter{ rootParameter }
{}


void BufferDescriptorSet::SetSRV(uint32_t srvRegister, ColorBufferPtr colorBuffer)
{
	const auto descriptor = (const Descriptor*)colorBuffer->GetSrvDescriptor();
	SetSRV_Internal(srvRegister, descriptor);
}


void BufferDescriptorSet::SetSRV(uint32_t srvRegister, DepthBufferPtr depthBuffer, bool depthSrv)
{
	const auto descriptor = (const Descriptor*)depthBuffer->GetSrvDescriptor(depthSrv);
	SetSRV_Internal(srvRegister, descriptor);
}


void BufferDescriptorSet::SetSRV(uint32_t srvRegister, GpuBufferPtr gpuBuffer)
{
	const auto descriptor = (const Descriptor*)gpuBuffer->GetSrvDescriptor();
	SetSRV_Internal(srvRegister, descriptor);
}


void BufferDescriptorSet::SetSRV(uint32_t srvRegister, TexturePtr texture)
{
	const auto descriptor = (const Descriptor*)texture->GetDescriptor();
	SetSRV_Internal(srvRegister, descriptor);
}


void BufferDescriptorSet::SetUAV(uint32_t uavRegister, ColorBufferPtr colorBuffer, uint32_t uavIndex)
{
	const auto descriptor = (const Descriptor*)colorBuffer->GetUavDescriptor();
	SetUAV_Internal(uavRegister, descriptor);
}


void BufferDescriptorSet::SetUAV(uint32_t uavRegister, DepthBufferPtr depthBuffer)
{
	assert_msg(false, "Depth UAVs not yet supported");
}


void BufferDescriptorSet::SetUAV(uint32_t uavRegister, GpuBufferPtr gpuBuffer)
{
	const auto descriptor = (const Descriptor*)gpuBuffer->GetUavDescriptor();
	SetUAV_Internal(uavRegister, descriptor);
}


void BufferDescriptorSet::SetCBV(uint32_t cbvRegister, GpuBufferPtr gpuBuffer)
{
	const auto descriptor = (const Descriptor*)gpuBuffer->GetCbvDescriptor();
	SetCBV_Internal(cbvRegister, descriptor);
}


void BufferDescriptorSet::SetSampler(uint32_t samplerRegister, SamplerPtr sampler)
{
	const auto descriptor = (const Descriptor*)sampler->GetDescriptor();
	SetSampler_Internal(samplerRegister, descriptor);
}


size_t BufferDescriptorSet::GetDescriptorBufferOffset() const
{
	return m_allocation.offsetInBytes;
}


void BufferDescriptorSet::SetSRV_Internal(uint32_t srvRegister, const Descriptor* descriptor)
{
	descriptor->CopyRawDescriptor((void*)(m_allocation.mem + GetRegisterOffsetSRV(srvRegister)));
}


void BufferDescriptorSet::SetUAV_Internal(uint32_t uavRegister, const Descriptor* descriptor)
{
	descriptor->CopyRawDescriptor((void*)(m_allocation.mem + GetRegisterOffsetUAV(uavRegister)));
}


void BufferDescriptorSet::SetCBV_Internal(uint32_t cbvRegister, const Descriptor* descriptor)
{
	descriptor->CopyRawDescriptor((void*)(m_allocation.mem + GetRegisterOffsetCBV(cbvRegister)));
}


void BufferDescriptorSet::SetSampler_Internal(uint32_t samplerRegister, const Descriptor* descriptor)
{
	descriptor->CopyRawDescriptor((void*)(m_allocation.mem + GetRegisterOffsetSampler(samplerRegister)));
}


size_t BufferDescriptorSet::GetRegisterOffsetSRV(uint32_t srvRegister, uint32_t arrayIndex, uint32_t descriptorSize) const
{
	const uint32_t bindingIndex = GetRegisterShiftSRV() + srvRegister;
	return m_layout->GetBindingOffset(bindingIndex) + arrayIndex * descriptorSize;
}


size_t BufferDescriptorSet::GetRegisterOffsetUAV(uint32_t uavRegister, uint32_t arrayIndex, uint32_t descriptorSize) const
{
	const uint32_t bindingIndex = GetRegisterShiftUAV() + uavRegister;
	return m_layout->GetBindingOffset(bindingIndex) + arrayIndex * descriptorSize;
}


size_t BufferDescriptorSet::GetRegisterOffsetCBV(uint32_t cbvRegister, uint32_t arrayIndex, uint32_t descriptorSize) const
{
	const uint32_t bindingIndex = GetRegisterShiftCBV() + cbvRegister;
	return m_layout->GetBindingOffset(bindingIndex) + arrayIndex * descriptorSize;
}


size_t BufferDescriptorSet::GetRegisterOffsetSampler(uint32_t samplerRegister, uint32_t arrayIndex, uint32_t descriptorSize) const
{
	const uint32_t bindingIndex = GetRegisterShiftSampler() + samplerRegister;
	return m_layout->GetBindingOffset(bindingIndex) + arrayIndex * descriptorSize;
}


HeapDescriptorSet::HeapDescriptorSet(Device* device, const RootParameter& rootParameter)
	: m_device{ device }
	, m_rootParameter{ rootParameter }
{}


void HeapDescriptorSet::SetSRV(uint32_t srvRegister, ColorBufferPtr colorBuffer)
{
	const auto descriptor = (const Descriptor*)colorBuffer->GetSrvDescriptor();
	SetSRV_Internal(srvRegister, descriptor);
}


void HeapDescriptorSet::SetSRV(uint32_t srvRegister, DepthBufferPtr depthBuffer, bool depthSrv)
{
	const auto descriptor = (const Descriptor*)depthBuffer->GetSrvDescriptor(depthSrv);
	SetSRV_Internal(srvRegister, descriptor);
}


void HeapDescriptorSet::SetSRV(uint32_t srvRegister, GpuBufferPtr gpuBuffer)
{
	const auto descriptor = (const Descriptor*)gpuBuffer->GetSrvDescriptor();
	SetSRV_Internal(srvRegister, descriptor);
}


void HeapDescriptorSet::SetSRV(uint32_t srvRegister, TexturePtr texture)
{
	const auto descriptor = (const Descriptor*)texture->GetDescriptor();
	SetSRV_Internal(srvRegister, descriptor);
}


void HeapDescriptorSet::SetUAV(uint32_t uavRegister, ColorBufferPtr colorBuffer, uint32_t uavIndex)
{
	const auto descriptor = (const Descriptor*)colorBuffer->GetUavDescriptor();
	SetUAV_Internal(uavRegister, descriptor);
}


void HeapDescriptorSet::SetUAV(uint32_t uavRegister, DepthBufferPtr depthBuffer)
{
	assert_msg(false, "Depth UAVs not yet supported");
}


void HeapDescriptorSet::SetUAV(uint32_t uavRegister, GpuBufferPtr gpuBuffer)
{
	const auto descriptor = (const Descriptor*)gpuBuffer->GetUavDescriptor();
	SetUAV_Internal(uavRegister, descriptor);
}


void HeapDescriptorSet::SetCBV(uint32_t cbvRegister, GpuBufferPtr gpuBuffer)
{
	const auto descriptor = (const Descriptor*)gpuBuffer->GetCbvDescriptor();
	SetCBV_Internal(cbvRegister, descriptor);
}


void HeapDescriptorSet::SetSampler(uint32_t samplerRegister, SamplerPtr sampler)
{
	const auto descriptor = (const Descriptor*)sampler->GetDescriptor();
	SetSampler_Internal(samplerRegister, descriptor);
}


void HeapDescriptorSet::SetSRV_Internal(uint32_t srvRegister, const Descriptor* descriptor)
{
	const uint16_t rangeIndex = m_rootParameter.FindMatchingRangeIndex(DescriptorRegisterType::SRV, srvRegister);
	std::byte* mem = m_rangeAllocations[rangeIndex].mem;
	const uint32_t offset = m_registerOffsetMap->GetRegisterOffsetSRV(srvRegister);
	descriptor->CopyRawDescriptor(mem + offset * descriptor->GetRawDescriptorSize());
}


void HeapDescriptorSet::SetUAV_Internal(uint32_t uavRegister, const Descriptor* descriptor)
{
	const uint16_t rangeIndex = m_rootParameter.FindMatchingRangeIndex(DescriptorRegisterType::UAV, uavRegister);
	std::byte* mem = m_rangeAllocations[rangeIndex].mem;
	const uint32_t offset = m_registerOffsetMap->GetRegisterOffsetUAV(uavRegister);
	descriptor->CopyRawDescriptor(mem + offset * descriptor->GetRawDescriptorSize());
}


void HeapDescriptorSet::SetCBV_Internal(uint32_t cbvRegister, const Descriptor* descriptor)
{
	const uint16_t rangeIndex = m_rootParameter.FindMatchingRangeIndex(DescriptorRegisterType::CBV, cbvRegister);
	std::byte* mem = m_rangeAllocations[rangeIndex].mem;
	const uint32_t offset = m_registerOffsetMap->GetRegisterOffsetCBV(cbvRegister);
	descriptor->CopyRawDescriptor(mem + offset * descriptor->GetRawDescriptorSize());
}


void HeapDescriptorSet::SetSampler_Internal(uint32_t samplerRegister, const Descriptor* descriptor)
{
	const uint16_t rangeIndex = m_rootParameter.FindMatchingRangeIndex(DescriptorRegisterType::Sampler, samplerRegister);
	std::byte* mem = m_rangeAllocations[rangeIndex].mem;
	const uint32_t offset = m_registerOffsetMap->GetRegisterOffsetSampler(samplerRegister);
	descriptor->CopyRawDescriptor(mem + offset * descriptor->GetRawDescriptorSize());
}

} // namespace Luna::VK