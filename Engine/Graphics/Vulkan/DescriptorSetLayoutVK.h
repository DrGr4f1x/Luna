//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#pragma once

#include "Graphics\Vulkan\VulkanCommon.h"


namespace Luna::VK
{

struct BindingInfo
{
	VkDeviceSize offset{ 0 };
	VkDescriptorType type;
};


class DescriptorBindingTemplate
{
	friend class Device;

public:
	BindingInfo GetBindingInfo(uint32_t bindingIndex) const noexcept;

private:
	std::unordered_map<uint32_t, BindingInfo> m_bindingInfoMap;
};


class DescriptorSetLayout
{
	friend class Device;

public:

	wil::com_ptr<CVkDescriptorSetLayout> GetDescriptorSetLayout() const noexcept;

	size_t GetHashCode() const noexcept { return m_hashcode; }

protected:
	Device* m_device{ nullptr };
	wil::com_ptr<CVkDescriptorSetLayout> m_descriptorSetLayout;
	size_t m_hashcode{ 0 };

	// Descriptor buffer data
	VkDeviceSize m_layoutSize;
	std::shared_ptr<DescriptorBindingTemplate> m_bindingTemplate;
};

using DescriptorSetLayoutPtr = std::shared_ptr<DescriptorSetLayout>;

} // namespace Luna::VK