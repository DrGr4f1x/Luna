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

#include "DescriptorHeap12.h"
#include "Device12.h"


namespace Luna::DX12
{

Luna::DescriptorSetPtr RootSignature::CreateDescriptorSet(DescriptorHeapPtr descriptorHeap, uint32_t rootParamIndex) const
{
	const auto& rootParam = GetRootParameter(rootParamIndex);

	// Only create a descriptor set for tables
	assert(rootParam.parameterType == RootParameterType::Table);

	const bool isSamplerTable = rootParam.IsSamplerTable();
	const DescriptorHeapType heapType = isSamplerTable ? DescriptorHeapType::Sampler : DescriptorHeapType::Resource;

	const uint32_t numDescriptors = rootParam.GetNumDescriptors();

	DescriptorSetDesc descriptorSetDesc{
		.rootParameter = rootParam,
		.numDescriptors = numDescriptors,
		.isSamplerTable = isSamplerTable
	};

	DescriptorHeap* descriptorHeap12 = (DescriptorHeap*)descriptorHeap.get();
	descriptorSetDesc.descriptorHandle = descriptorHeap12->AllocateDescriptors(heapType, numDescriptors);
	

	return m_device->CreateDescriptorSet(descriptorSetDesc);
}


uint32_t RootSignature::GetDescriptorOffsetForRegister(DescriptorRegisterType type, uint32_t rootIndex, uint32_t descriptorRegister) const
{
	assert(rootIndex < MaxRootParameters);
	assert(m_descriptorOffsetTables[rootIndex]);
	return m_descriptorOffsetTables[rootIndex]->GetDescriptorOffset(type, descriptorRegister);
}

} // namespace Luna::DX12