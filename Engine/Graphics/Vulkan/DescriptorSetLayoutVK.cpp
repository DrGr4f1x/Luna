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

#include "DescriptorSetLayoutVK.h"


namespace Luna::VK
{

BindingInfo DescriptorBindingTemplate::GetBindingInfo(uint32_t bindingIndex) const noexcept
{
	auto res = m_bindingInfoMap.find(bindingIndex);
	assert(res != m_bindingInfoMap.end());
	return res->second;
}


wil::com_ptr<CVkDescriptorSetLayout> DescriptorSetLayout::GetDescriptorSetLayout() const noexcept
{
	return m_descriptorSetLayout;
}


VkDeviceSize DescriptorSetLayout::GetBindingOffset(uint32_t bindingIndex) const
{
	return m_bindingTemplate->GetBindingInfo(bindingIndex).offset;
}

} // namespace Luna::VK