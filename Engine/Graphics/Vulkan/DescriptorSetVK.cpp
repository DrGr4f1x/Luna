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

DescriptorSet::DescriptorSet(Device* device, const RootParameter& rootParameter)
	: m_device{ device }
	, m_rootParameter{ rootParameter }
{}


void DescriptorSet::SetBindlessSRVs(uint32_t srvRegister, std::span<const IDescriptor*> descriptors)
{
	if (m_useDescriptorHeap)
	{
		SetDescriptors_Internal_Heap<DescriptorRegisterType::SRV>(srvRegister, descriptors);
	}
	else
	{
		SetDescriptors_Internal<DescriptorRegisterType::SRV>(srvRegister, descriptors);
	}
}


void DescriptorSet::SetSRV(uint32_t srvRegister, ColorBufferPtr colorBuffer)
{
	const auto descriptor = (const Descriptor*)colorBuffer->GetSrvDescriptor();
	SetSRV_Internal(srvRegister, descriptor);
}


void DescriptorSet::SetSRV(uint32_t srvRegister, DepthBufferPtr depthBuffer, bool depthSrv)
{
	const auto descriptor = (const Descriptor*)depthBuffer->GetSrvDescriptor(depthSrv);
	SetSRV_Internal(srvRegister, descriptor);
}


void DescriptorSet::SetSRV(uint32_t srvRegister, GpuBufferPtr gpuBuffer)
{
	const auto descriptor = (const Descriptor*)gpuBuffer->GetSrvDescriptor();
	SetSRV_Internal(srvRegister, descriptor);
}


void DescriptorSet::SetSRV(uint32_t srvRegister, TexturePtr texture)
{
	const auto descriptor = (const Descriptor*)texture->GetDescriptor();
	SetSRV_Internal(srvRegister, descriptor);
}


void DescriptorSet::SetUAV(uint32_t uavRegister, ColorBufferPtr colorBuffer, uint32_t uavIndex)
{
	const auto descriptor = (const Descriptor*)colorBuffer->GetUavDescriptor();
	SetUAV_Internal(uavRegister, descriptor);
}


void DescriptorSet::SetUAV(uint32_t uavRegister, DepthBufferPtr depthBuffer)
{
	assert_msg(false, "Depth UAVs not yet supported");
}


void DescriptorSet::SetUAV(uint32_t uavRegister, GpuBufferPtr gpuBuffer)
{
	const auto descriptor = (const Descriptor*)gpuBuffer->GetUavDescriptor();
	SetUAV_Internal(uavRegister, descriptor);
}


void DescriptorSet::SetCBV(uint32_t cbvRegister, GpuBufferPtr gpuBuffer)
{
	const auto descriptor = (const Descriptor*)gpuBuffer->GetCbvDescriptor();
	SetCBV_Internal(cbvRegister, descriptor);
}


void DescriptorSet::SetSampler(uint32_t samplerRegister, SamplerPtr sampler)
{
	const auto descriptor = (const Descriptor*)sampler->GetDescriptor();
	SetSampler_Internal(samplerRegister, descriptor);
}


size_t DescriptorSet::GetDescriptorBufferOffset() const
{
	return m_allocation.offsetInBytes;
}


template <DescriptorRegisterType registerType>
void DescriptorSet::SetDescriptors_Internal(uint32_t descriptorRegister, std::span<const IDescriptor*> descriptors)
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


template <DescriptorRegisterType registerType>
void DescriptorSet::SetDescriptors_Internal_Heap(uint32_t descriptorRegister, std::span<const IDescriptor*> descriptors)
{
	const uint32_t rangeIndex = m_rootParameter.FindMatchingRangeIndex(registerType, descriptorRegister);

	assert(rangeIndex != ~0u);
	const auto& range = m_rootParameter.table[rangeIndex];

	const size_t numDescriptors = std::min<size_t>(range.numDescriptors, descriptors.size());

	for (uint32_t i = 0; i < numDescriptors; ++i)
	{
		const auto descriptor = (const Descriptor*)descriptors[i];

		switch (range.descriptorType)
		{
		case DescriptorType::Sampler:
			descriptor->CopyRawDescriptor((void*)m_samplerRegisterOffsetMap[descriptorRegister + i]);
			break;

		case DescriptorType::ConstantBuffer:
			descriptor->CopyRawDescriptor((void*)m_cbvRegisterOffsetMap[descriptorRegister + i]);
			break;

		case DescriptorType::TextureSRV:
		case DescriptorType::TypedBufferSRV:
		case DescriptorType::StructuredBufferSRV:
		case DescriptorType::RawBufferSRV:
			descriptor->CopyRawDescriptor((void*)m_srvRegisterOffsetMap[descriptorRegister + i]);
			break;

		case DescriptorType::TextureUAV:
		case DescriptorType::TypedBufferUAV:
		case DescriptorType::StructuredBufferUAV:
		case DescriptorType::RawBufferUAV:
			descriptor->CopyRawDescriptor((void*)m_uavRegisterOffsetMap[descriptorRegister + i]);
			break;

		default:
			assert(false);
			break;
		}
	}
}


void DescriptorSet::SetSRV_Internal(uint32_t srvRegister, const Descriptor* descriptor)
{
	if (m_useDescriptorHeap)
	{
		descriptor->CopyRawDescriptor((void*)m_srvRegisterOffsetMap[srvRegister]);
	}
	else
	{
		descriptor->CopyRawDescriptor((void*)(m_allocation.mem + GetRegisterOffsetSRV(srvRegister)));
	}
}


void DescriptorSet::SetUAV_Internal(uint32_t uavRegister, const Descriptor* descriptor)
{
	if (m_useDescriptorHeap)
	{
		descriptor->CopyRawDescriptor((void*)m_uavRegisterOffsetMap[uavRegister]);
	}
	else
	{
		descriptor->CopyRawDescriptor((void*)(m_allocation.mem + GetRegisterOffsetUAV(uavRegister)));
	}
}


void DescriptorSet::SetCBV_Internal(uint32_t cbvRegister, const Descriptor* descriptor)
{
	if (m_useDescriptorHeap)
	{
		descriptor->CopyRawDescriptor((void*)m_cbvRegisterOffsetMap[cbvRegister]);
	}
	else
	{
		descriptor->CopyRawDescriptor((void*)(m_allocation.mem + GetRegisterOffsetCBV(cbvRegister)));
	}
}


void DescriptorSet::SetSampler_Internal(uint32_t samplerRegister, const Descriptor* descriptor)
{
	if (m_useDescriptorHeap)
	{
		descriptor->CopyRawDescriptor((void*)m_samplerRegisterOffsetMap[samplerRegister]);
	}
	else
	{
		descriptor->CopyRawDescriptor((void*)(m_allocation.mem + GetRegisterOffsetSampler(samplerRegister)));
	}
}


size_t DescriptorSet::GetRegisterOffsetSRV(uint32_t srvRegister, uint32_t arrayIndex, uint32_t descriptorSize) const
{
	assert(!m_useDescriptorHeap);

	const uint32_t bindingIndex = GetRegisterShiftSRV() + srvRegister;
	return m_layout->GetBindingOffset(bindingIndex) + arrayIndex * descriptorSize;
}


size_t DescriptorSet::GetRegisterOffsetUAV(uint32_t uavRegister, uint32_t arrayIndex, uint32_t descriptorSize) const
{
	assert(!m_useDescriptorHeap);

	const uint32_t bindingIndex = GetRegisterShiftUAV() + uavRegister;
	return m_layout->GetBindingOffset(bindingIndex) + arrayIndex * descriptorSize;
}


size_t DescriptorSet::GetRegisterOffsetCBV(uint32_t cbvRegister, uint32_t arrayIndex, uint32_t descriptorSize) const
{
	const uint32_t bindingIndex = GetRegisterShiftCBV() + cbvRegister;
	return m_layout->GetBindingOffset(bindingIndex) + arrayIndex * descriptorSize;
}


size_t DescriptorSet::GetRegisterOffsetSampler(uint32_t samplerRegister, uint32_t arrayIndex, uint32_t descriptorSize) const
{
	const uint32_t bindingIndex = GetRegisterShiftSampler() + samplerRegister;
	return m_layout->GetBindingOffset(bindingIndex) + arrayIndex * descriptorSize;
}

} // namespace Luna::VK