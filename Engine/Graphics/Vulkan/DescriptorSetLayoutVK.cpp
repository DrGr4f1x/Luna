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

size_t DescriptorBindingTemplate::GetOffset(uint32_t bindingIndex) const noexcept
{
	assert(bindingIndex < m_offsets.size());
	return m_offsets[bindingIndex];
}


wil::com_ptr<CVkDescriptorSetLayout> DescriptorSetLayout::GetDescriptorSetLayout() const noexcept
{
	return m_descriptorSetLayout;
}

} // namespace Luna::VK