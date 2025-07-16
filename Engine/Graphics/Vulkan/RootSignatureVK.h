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

#include "Graphics\RootSignature.h"
#include "Graphics\Vulkan\VulkanCommon.h"

namespace Luna::VK
{

// Forward declarations
class Device;


struct DescriptorBindingDesc
{
	VkDescriptorType descriptorType;
	uint32_t startSlot{ 0 };
	uint32_t numDescriptors{ 1 };
	uint32_t offset{ 0 };
};


class RootSignature : public IRootSignature
{
	friend class Device;

public:
	Luna::DescriptorSetPtr CreateDescriptorSet(uint32_t rootParamIndex) const override;

	VkPipelineLayout GetPipelineLayout() const noexcept { return m_pipelineLayout->Get(); }
	CVkDescriptorSetLayout* GetDescriptorSetLayout(uint32_t rootParamIndex) const noexcept { return m_descriptorSetLayouts[rootParamIndex].get(); }
	const std::vector<DescriptorBindingDesc>& GetLayoutBindings(uint32_t rootParamIndex) const;

protected:
	Device* m_device{ nullptr };
	wil::com_ptr<CVkPipelineLayout> m_pipelineLayout;
	std::unordered_map<uint32_t, std::vector<DescriptorBindingDesc>> m_layoutBindingMap;
	std::unordered_map<uint32_t, uint32_t> m_rootParameterIndexToDescriptorSetMap;
	std::vector<wil::com_ptr<CVkDescriptorSetLayout>> m_descriptorSetLayouts;
};

} // namespace Luna::VK