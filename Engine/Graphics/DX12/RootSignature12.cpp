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

#include "RootSignature12.h"

#include "Device12.h"


namespace Luna::DX12
{

Luna::DescriptorSetPtr RootSignature::CreateDescriptorSet(uint32_t rootParamIndex) const
{
	const auto& rootParam = GetRootParameter(rootParamIndex);

	// Don't need descriptors for root constants
	assert(rootParam.parameterType != RootParameterType::RootConstants);

	const bool isRootBuffer = rootParam.parameterType == RootParameterType::RootCBV ||
		rootParam.parameterType == RootParameterType::RootSRV ||
		rootParam.parameterType == RootParameterType::RootUAV;

	const bool isSamplerTable = rootParam.IsSamplerTable();

	const D3D12_DESCRIPTOR_HEAP_TYPE heapType = isSamplerTable
		? D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
		: D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	const uint32_t numDescriptors = rootParam.GetNumDescriptors();
	DescriptorSetDesc descriptorSetDesc{
		.descriptorHandle = isRootBuffer ? DescriptorHandle{} : AllocateUserDescriptor(heapType, numDescriptors),
		.numDescriptors = numDescriptors,
		.isSamplerTable = isSamplerTable,
		.isRootBuffer = isRootBuffer
	};

	return m_device->CreateDescriptorSet(descriptorSetDesc);
}

} // namespace Luna::DX12