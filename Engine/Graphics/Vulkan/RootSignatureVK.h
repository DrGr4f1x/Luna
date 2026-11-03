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


struct PushDataEntry
{
	uint16_t offset{ 0 };
	uint16_t size{ 0 };
};

using PushDataLayout = std::vector<std::vector<PushDataEntry>>;


struct BindingMappingAndInfo
{
	std::vector<VkDescriptorSetAndBindingMappingEXT> bindings;
	VkShaderDescriptorSetAndBindingMappingInfoEXT info;

	void Finalize();
};

struct ShaderSetAndBindingMapping
{
	BindingMappingAndInfo compute;
	BindingMappingAndInfo vertex;
	BindingMappingAndInfo hull;
	BindingMappingAndInfo domain;
	BindingMappingAndInfo geometry;
	BindingMappingAndInfo pixel;
	BindingMappingAndInfo amplification;
	BindingMappingAndInfo mesh;

	void Finalize();
};


class RootSignature : public IRootSignature
{
	friend class Device;

public:
	DescriptorSetPtr CreateDescriptorSet(DescriptorHeapPtr descriptorHeap, uint32_t rootParamIndex) const override;

	VkPipelineLayout GetPipelineLayout() const noexcept { return m_pipelineLayout->Get(); }
	DescriptorSetLayout* GetDescriptorSetLayout(uint32_t rootParamIndex) const noexcept;

	bool HasStaticSamplers() const noexcept { return m_staticSamplerDescriptorSetIndex != ~0u; }

	size_t GetResourceDescriptorSetLayoutSize() const noexcept { return m_resourceDescriptorSetLayoutSize; }
	size_t GetSamplerDescriptorSetLayoutSize() const noexcept { return m_samplerDescriptorSetLayoutSize; }

	uint32_t GetStaticSamplerDescriptorSetIndex() const noexcept { return m_staticSamplerDescriptorSetIndex; }

	uint32_t GetPushDescriptorSetIndex() const noexcept { return m_pushDescriptorSetIndex; }
	uint32_t GetPushDescriptorBinding(uint32_t rootParamIndex) const noexcept;

	// Push data (for DescriptorHeap)
	const PushDataLayout& GetPushDataLayout() const noexcept { return m_pushDataLayout; }
	const std::vector<PushDataEntry>& GetPushDataEntries(uint32_t index) const { return m_pushDataLayout[index]; }
	const PushDataEntry& GetPushDataEntry(uint32_t index, uint32_t entryIndex) const { return m_pushDataLayout[index][entryIndex]; }
	uint32_t GetPushDataSize() const noexcept { return m_pushDataSize; }

	// Shader set and binding mapping (for DescriptorHeap)
	const ShaderSetAndBindingMapping& GetShaderSetAndBindingMapping() const { return m_shaderSetAndBindingMapping; }

protected:
	Device* m_device{ nullptr };
	wil::com_ptr<CVkPipelineLayout> m_pipelineLayout;
	std::vector<DescriptorSetLayoutPtr> m_descriptorSetLayouts;
	size_t m_resourceDescriptorSetLayoutSize{ 0 };
	size_t m_samplerDescriptorSetLayoutSize{ 0 };

	// Static samplers
	std::vector<SamplerPtr> m_staticSamplers;
	uint32_t m_staticSamplerDescriptorSetIndex{ ~0u };

	// Push descriptors
	uint32_t m_pushDescriptorSetIndex{ (uint32_t)-1 };
	std::unordered_map<uint32_t, uint32_t> m_pushDescriptorBindingMap;

	// Push data
	PushDataLayout m_pushDataLayout;
	uint32_t m_pushDataSize{ 0 };

	// Shader set and binding mapping
	ShaderSetAndBindingMapping m_shaderSetAndBindingMapping;

	// Is using descriptor heaps?
	bool m_useDescriptorHeap{ false };
};

} // namespace Luna::VK