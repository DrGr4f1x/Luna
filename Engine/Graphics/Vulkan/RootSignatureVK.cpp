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

	DescriptorSetDesc descriptorSetDesc{
		.descriptorSetLayout	= m_descriptorSetLayouts[rootParamIndex].get(),
		.rootParameter			= rootParam,
		.numDescriptors			= rootParam.GetNumDescriptors()
	};

	return m_device->CreateDescriptorSet(descriptorSetDesc);
}


const std::vector<DescriptorBindingDesc>& RootSignature::GetLayoutBindings(uint32_t rootParamIndex) const
{
	const auto it = m_layoutBindingMap.find(rootParamIndex);
	assert(it != m_layoutBindingMap.end());

	return it->second;
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