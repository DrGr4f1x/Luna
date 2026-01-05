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

#include "RootSignatureVK.h"

#include "DeviceVK.h"


namespace Luna::VK
{

Luna::DescriptorSetPtr RootSignature::CreateDescriptorSet(uint32_t rootParamIndex) const
{
	const auto& rootParam = GetRootParameter(rootParamIndex);

	// Can only create descriptor sets for tables
	assert(rootParam.parameterType == RootParameterType::Table);
	
#if USE_DESCRIPTOR_BUFFERS
	DescriptorSetDesc descriptorSetDesc{
		.descriptorSetLayout	= m_descriptorSetLayouts[rootParamIndex],
		.rootParameter			= rootParam
	};
#endif // USE_DESCRIPTOR_BUFFERS

#if USE_LEGACY_DESCRIPTOR_SETS
	DescriptorSetDesc descriptorSetDesc{
		.descriptorSetLayout	= m_descriptorSetLayouts[rootParamIndex]->GetDescriptorSetLayout().get(),
		.rootParameter			= rootParam,
		.numDescriptors			= rootParam.GetNumDescriptors()
	};
#endif // USE_LEGACY_DESCRIPTOR_SETS

	return m_device->CreateDescriptorSet(descriptorSetDesc);
}


DescriptorSetLayout* RootSignature::GetDescriptorSetLayout(uint32_t rootParamIndex) const noexcept
{
	if (rootParamIndex < m_descriptorSetLayouts.size())
	{
		return m_descriptorSetLayouts[rootParamIndex].get();
	}
	return nullptr;
}


uint32_t RootSignature::GetPushDescriptorBinding(uint32_t rootParamIndex) const noexcept
{
	auto it = m_pushDescriptorBindingMap.find(rootParamIndex);
	if (it != m_pushDescriptorBindingMap.end())
	{
		return it->second;
	}

	return (uint32_t)-1;
}

} // namespace Luna::VK