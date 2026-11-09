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
	VkShaderDescriptorSetAndBindingMappingInfoEXT info{ VK_STRUCTURE_TYPE_SHADER_DESCRIPTOR_SET_AND_BINDING_MAPPING_INFO_EXT };

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

	std::vector<VkSamplerCreateInfo> staticSamplers;

	void Add(const VkDescriptorSetAndBindingMappingEXT& setAndBindingMapping, ShaderStage shaderStage);

	void Finalize();
};


class IRootSignatureVK : public IRootSignature
{
public:
	virtual DescriptorManagement GetDescriptorManagement() const = 0;

	virtual VkPipelineLayout GetPipelineLayout() const = 0;
};


class BufferRootSignature : public IRootSignatureVK
{
	friend class Device;

public:
	// IRootSignatureVK API
	DescriptorManagement GetDescriptorManagement() const final { return DescriptorManagement::Buffer; }

	// IRootSignature API
	DescriptorSetPtr CreateDescriptorSet(DescriptorHeapPtr descriptorHeap, uint32_t rootParamIndex) const override;

	VkPipelineLayout GetPipelineLayout() const final { return m_pipelineLayout->Get(); }

	// Descriptor buffer API
	DescriptorSetLayout* GetDescriptorSetLayout(uint32_t rootParamIndex) const noexcept;

	bool HasStaticSamplers() const noexcept { return m_staticSamplerDescriptorSetIndex != ~0u; }

	size_t GetResourceDescriptorSetLayoutSize() const noexcept { return m_resourceDescriptorSetLayoutSize; }
	size_t GetSamplerDescriptorSetLayoutSize() const noexcept { return m_samplerDescriptorSetLayoutSize; }

	uint32_t GetStaticSamplerDescriptorSetIndex() const noexcept { return m_staticSamplerDescriptorSetIndex; }

	uint32_t GetPushDescriptorSetIndex() const noexcept { return m_pushDescriptorSetIndex; }
	uint32_t GetPushDescriptorBinding(uint32_t rootParamIndex) const noexcept;

private:
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
};


class HeapRootSignature : public IRootSignatureVK
{
	friend class Device;

public:
	// IRootSignatureVK API
	DescriptorManagement GetDescriptorManagement() const final { return DescriptorManagement::Heap; }

	// IRootSignature API
	DescriptorSetPtr CreateDescriptorSet(DescriptorHeapPtr descriptorHeap, uint32_t rootParamIndex) const override;

	VkPipelineLayout GetPipelineLayout() const final { return VK_NULL_HANDLE; }

	// Descriptor heap API
	
	// Push data
	const PushDataLayout& GetPushDataLayout() const noexcept { return m_pushDataLayout; }
	const std::vector<PushDataEntry>& GetPushDataEntries(uint32_t index) const { return m_pushDataLayout[index]; }
	const PushDataEntry& GetPushDataEntry(uint32_t index, uint32_t entryIndex) const { return m_pushDataLayout[index][entryIndex]; }
	uint32_t GetPushDataSize() const noexcept { return m_pushDataSize; }

	// Shader set and binding mapping
	const ShaderSetAndBindingMapping& GetShaderSetAndBindingMapping() const { return m_shaderSetAndBindingMapping; }

private:
	Device* m_device{ nullptr };

	// Push data
	PushDataLayout m_pushDataLayout;
	uint32_t m_pushDataSize{ 0 };

	// Shader set and binding mapping
	ShaderSetAndBindingMapping m_shaderSetAndBindingMapping;

	// Register offset maps
	std::vector<HeapDescriptorBindingMapPtr> m_registerOffsetMaps;
};

} // namespace Luna::VK