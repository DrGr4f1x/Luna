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
#include "Graphics\Vulkan\DescriptorSetLayoutVK.h"

namespace Luna::VK
{

// Forward declarations
class Device;


class RootSignature : public IRootSignature
{
	friend class Device;

public:
	DescriptorSetPtr CreateDescriptorSet(uint32_t rootParamIndex) const override;
	DescriptorSetPtr CreateDescriptorSet2(uint32_t rootParamIndex) const override;

	VkPipelineLayout GetPipelineLayout() const noexcept { return m_pipelineLayout->Get(); }
	DescriptorSetLayout* GetDescriptorSetLayout(uint32_t rootParamIndex) const noexcept;

	uint32_t GetStaticSamplerDescriptorSetIndex() const noexcept { return m_staticSamplerDescriptorSetIndex; }
	VkDescriptorSet GetStaticSamplerDescriptorSet() const noexcept { return m_staticSamplerDescriptorSet; }

	uint32_t GetPushDescriptorSetIndex() const noexcept { return m_pushDescriptorSetIndex; }
	uint32_t GetPushDescriptorBinding(uint32_t rootParamIndex) const noexcept;

protected:
	Device* m_device{ nullptr };
	wil::com_ptr<CVkPipelineLayout> m_pipelineLayout;
	std::unordered_map<uint32_t, uint32_t> m_rootParameterIndexToDescriptorSetMap;
	std::vector<DescriptorSetLayoutPtr> m_descriptorSetLayouts;

	// Static samplers
	std::vector<SamplerPtr> m_staticSamplers;
	uint32_t m_staticSamplerDescriptorSetIndex{ 0 };
	VkDescriptorSet m_staticSamplerDescriptorSet{ VK_NULL_HANDLE };

	// Push descriptors
	uint32_t m_pushDescriptorSetIndex{ (uint32_t)-1 };
	std::unordered_map<uint32_t, uint32_t> m_pushDescriptorBindingMap;
};

} // namespace Luna::VK