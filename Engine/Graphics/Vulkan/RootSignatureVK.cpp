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

	const bool isDynamicBuffer = rootParam.parameterType == RootParameterType::RootCBV ||
		rootParam.parameterType == RootParameterType::RootSRV ||
		rootParam.parameterType == RootParameterType::RootUAV;

	DescriptorSetDesc descriptorSetDesc{
		.descriptorSetLayout	= m_descriptorSetLayouts[rootParamIndex].get(),
		.rootParameter			= rootParam,
		.numDescriptors			= rootParam.GetNumDescriptors(),
		.isDynamicBuffer		= isDynamicBuffer
	};

	return m_device->CreateDescriptorSet(descriptorSetDesc);
}


const std::vector<DescriptorBindingDesc>& RootSignature::GetLayoutBindings(uint32_t rootParamIndex) const
{
	const auto it = m_layoutBindingMap.find(rootParamIndex);
	assert(it != m_layoutBindingMap.end());

	return it->second;
}

} // namespace Luna::VK