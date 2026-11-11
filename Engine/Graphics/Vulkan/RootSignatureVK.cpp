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

#include "DescriptorHeapVK.h"
#include "DeviceVK.h"


namespace Luna::VK
{

void BindingMappingAndInfo::Finalize()
{
	info.sType = VK_STRUCTURE_TYPE_SHADER_DESCRIPTOR_SET_AND_BINDING_MAPPING_INFO_EXT;
	info.pNext = nullptr;
	info.mappingCount = (uint32_t)bindings.size();
	info.pMappings = bindings.data();
}


void ShaderSetAndBindingMapping::Add(const VkDescriptorSetAndBindingMappingEXT& setAndBindingMapping, ShaderStage shaderStage)
{
	auto AddMapping = [setAndBindingMapping, shaderStage](std::vector<VkDescriptorSetAndBindingMappingEXT>& shaderSet, const ShaderStage target)
		{
			if (HasFlag(shaderStage, target))
			{
				shaderSet.push_back(setAndBindingMapping);
			}
		};

	AddMapping(compute.bindings, ShaderStage::Compute);
	AddMapping(vertex.bindings, ShaderStage::Vertex);
	AddMapping(hull.bindings, ShaderStage::Hull);
	AddMapping(domain.bindings, ShaderStage::Domain);
	AddMapping(geometry.bindings, ShaderStage::Geometry);
	AddMapping(pixel.bindings, ShaderStage::Pixel);
	AddMapping(amplification.bindings, ShaderStage::Amplification);
	AddMapping(mesh.bindings, ShaderStage::Mesh);
}


void ShaderSetAndBindingMapping::Finalize()
{
	compute.Finalize();
	vertex.Finalize();
	hull.Finalize();
	domain.Finalize();
	geometry.Finalize();
	pixel.Finalize();
	amplification.Finalize();
	mesh.Finalize();
}


Luna::DescriptorSetPtr BufferRootSignature::CreateDescriptorSet(uint32_t rootParamIndex) const
{
	IDescriptorHeapVK* descriptorHeapVK = (IDescriptorHeapVK*)m_descriptorHeap.get();

	const auto& rootParam = GetRootParameter(rootParamIndex);

	// Can only create descriptor sets for tables
	assert(rootParam.parameterType == RootParameterType::Table);
	
	DescriptorSetDesc descriptorSetDesc{
		.descriptorSetLayout	= m_descriptorSetLayouts[rootParamIndex],
		.rootParameter			= rootParam
	};

	return m_device->CreateDescriptorSet(descriptorHeapVK, descriptorSetDesc);
}


DescriptorSetLayout* BufferRootSignature::GetDescriptorSetLayout(uint32_t rootParamIndex) const noexcept
{
	if (rootParamIndex < m_descriptorSetLayouts.size())
	{
		return m_descriptorSetLayouts[rootParamIndex].get();
	}
	
	return nullptr;
}


uint32_t BufferRootSignature::GetPushDescriptorBinding(uint32_t rootParamIndex) const noexcept
{
	auto it = m_pushDescriptorBindingMap.find(rootParamIndex);
	if (it != m_pushDescriptorBindingMap.end())
	{
		return it->second;
	}

	return (uint32_t)-1;
}


Luna::DescriptorSetPtr HeapRootSignature::CreateDescriptorSet(uint32_t rootParamIndex) const
{
	IDescriptorHeapVK* descriptorHeapVK = (IDescriptorHeapVK*)m_descriptorHeap.get();

	const auto& rootParam = GetRootParameter(rootParamIndex);

	// Can only create descriptor sets for tables
	assert(rootParam.parameterType == RootParameterType::Table);

	return m_device->CreateDescriptorSet(descriptorHeapVK, m_pushDataLayout[rootParamIndex], m_registerOffsetMaps[rootParamIndex], rootParam);
}

} // namespace Luna::VK