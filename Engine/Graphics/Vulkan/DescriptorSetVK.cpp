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


void BufferDescriptorSet::SetBindlessSRVs(uint32_t srvRegister, std::span<const IDescriptor*> descriptors)
{
	SetDescriptors_Internal<DescriptorRegisterType::SRV>(srvRegister, descriptors);
}


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


template <DescriptorRegisterType registerType>
void BufferDescriptorSet::SetDescriptors_Internal(uint32_t descriptorRegister, std::span<const IDescriptor*> descriptors)
{
	const uint32_t rangeIndex = m_rootParameter.FindMatchingRangeIndex(registerType, descriptorRegister);

	assert(rangeIndex != ~0u);
	const auto& range = m_rootParameter.table[rangeIndex];

	const size_t numDescriptors = std::min<size_t>(range.numDescriptors, descriptors.size());

	for (uint32_t i = 0; i < numDescriptors; ++i)
	{
		const auto descriptor = (const Descriptor*)descriptors[i];
		const uint32_t descriptorSize = (uint32_t)descriptor->GetRawDescriptorSize();

		switch (range.descriptorType)
		{
		case DescriptorType::Sampler:
			descriptor->CopyRawDescriptor((void*)(m_allocation.mem + GetRegisterOffsetSampler(descriptorRegister, i, descriptorSize)));
			break;

		case DescriptorType::ConstantBuffer:
			descriptor->CopyRawDescriptor((void*)(m_allocation.mem + GetRegisterOffsetCBV(descriptorRegister, i, descriptorSize)));
			break;

		case DescriptorType::TextureSRV:
		case DescriptorType::TypedBufferSRV:
		case DescriptorType::StructuredBufferSRV:
		case DescriptorType::RawBufferSRV:
			descriptor->CopyRawDescriptor((void*)(m_allocation.mem + GetRegisterOffsetSRV(descriptorRegister, i, descriptorSize)));
			break;

		case DescriptorType::TextureUAV:
		case DescriptorType::TypedBufferUAV:
		case DescriptorType::StructuredBufferUAV:
		case DescriptorType::RawBufferUAV:
			descriptor->CopyRawDescriptor((void*)(m_allocation.mem + GetRegisterOffsetUAV(descriptorRegister, i, descriptorSize)));
			break;

		default:
			assert(false);
			break;
		}
	}
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


void HeapDescriptorSet::SetBindlessSRVs(uint32_t srvRegister, std::span<const IDescriptor*> descriptors)
{
	SetDescriptors_Internal<DescriptorRegisterType::SRV>(srvRegister, descriptors);
}


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


template <DescriptorRegisterType registerType>
void HeapDescriptorSet::SetDescriptors_Internal(uint32_t descriptorRegister, std::span<const IDescriptor*> descriptors)
{
	const uint32_t rangeIndex = m_rootParameter.FindMatchingRangeIndex(registerType, descriptorRegister);

	assert(rangeIndex != ~0u);
	const auto& range = m_rootParameter.table[rangeIndex];
	std::byte* mem = m_rangeAllocations[rangeIndex].mem;

	const size_t numDescriptors = std::min<size_t>(range.numDescriptors, descriptors.size());

	for (uint32_t i = 0; i < numDescriptors; ++i)
	{
		const auto descriptor = (const Descriptor*)descriptors[i];
		const size_t descriptorSize = descriptor->GetRawDescriptorSize();

		switch (range.descriptorType)
		{
		case DescriptorType::Sampler:
			descriptor->CopyRawDescriptor(mem + m_samplerRegisterOffsetMap[descriptorRegister + i] * descriptorSize);
			break;

		case DescriptorType::ConstantBuffer:
			descriptor->CopyRawDescriptor(mem + m_cbvRegisterOffsetMap[descriptorRegister + i] * descriptorSize);
			break;

		case DescriptorType::TextureSRV:
		case DescriptorType::TypedBufferSRV:
		case DescriptorType::StructuredBufferSRV:
		case DescriptorType::RawBufferSRV:
			descriptor->CopyRawDescriptor(mem + m_srvRegisterOffsetMap[descriptorRegister + i] * descriptorSize);
			break;

		case DescriptorType::TextureUAV:
		case DescriptorType::TypedBufferUAV:
		case DescriptorType::StructuredBufferUAV:
		case DescriptorType::RawBufferUAV:
			descriptor->CopyRawDescriptor(mem + m_uavRegisterOffsetMap[descriptorRegister + i] * descriptorSize);
			break;

		default:
			assert(false);
			break;
		}
	}
}


void HeapDescriptorSet::SetSRV_Internal(uint32_t srvRegister, const Descriptor* descriptor)
{
	const uint16_t rangeIndex = m_rootParameter.FindMatchingRangeIndex(DescriptorRegisterType::SRV, srvRegister);
	std::byte* mem = m_rangeAllocations[rangeIndex].mem;
	descriptor->CopyRawDescriptor(mem + m_srvRegisterOffsetMap[srvRegister] * descriptor->GetRawDescriptorSize());
}


void HeapDescriptorSet::SetUAV_Internal(uint32_t uavRegister, const Descriptor* descriptor)
{
	const uint16_t rangeIndex = m_rootParameter.FindMatchingRangeIndex(DescriptorRegisterType::UAV, uavRegister);
	std::byte* mem = m_rangeAllocations[rangeIndex].mem;
	descriptor->CopyRawDescriptor(mem + m_uavRegisterOffsetMap[uavRegister] * descriptor->GetRawDescriptorSize());
}


void HeapDescriptorSet::SetCBV_Internal(uint32_t cbvRegister, const Descriptor* descriptor)
{
	const uint16_t rangeIndex = m_rootParameter.FindMatchingRangeIndex(DescriptorRegisterType::CBV, cbvRegister);
	std::byte* mem = m_rangeAllocations[rangeIndex].mem;
	descriptor->CopyRawDescriptor(mem + m_cbvRegisterOffsetMap[cbvRegister] * descriptor->GetRawDescriptorSize());
}


void HeapDescriptorSet::SetSampler_Internal(uint32_t samplerRegister, const Descriptor* descriptor)
{
	const uint16_t rangeIndex = m_rootParameter.FindMatchingRangeIndex(DescriptorRegisterType::Sampler, samplerRegister);
	std::byte* mem = m_rangeAllocations[rangeIndex].mem;
	descriptor->CopyRawDescriptor(mem + m_samplerRegisterOffsetMap[samplerRegister] * descriptor->GetRawDescriptorSize());
}

} // namespace Luna::VK