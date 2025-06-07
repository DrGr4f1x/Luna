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

#include "RootSignatureFactoryVK.h"

#include "Graphics\ResourceManager.h"


namespace Luna::VK
{

inline VkDescriptorType RootParameterTypeToVulkanDescriptorType(RootParameterType rootParameterType)
{
	switch (rootParameterType)
	{
	case RootParameterType::RootCBV: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	case RootParameterType::RootSRV:
	case RootParameterType::RootUAV: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
	default:						 return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}
}


inline uint32_t GetDescriptorOffset(VulkanBindingOffsets bindingOffsets, VkDescriptorType descriptorType)
{
	switch (descriptorType)
	{
	case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
	case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
	case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
	case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:	return bindingOffsets.shaderResource;

	case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
	case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
	case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
	case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:		return bindingOffsets.unorderedAccess;

	case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
	case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:		return bindingOffsets.constantBuffer;

	case VK_DESCRIPTOR_TYPE_SAMPLER:					return bindingOffsets.sampler;
	}
	return 0;
}


RootSignatureFactory::RootSignatureFactory(IResourceManager* owner, CVkDevice* device)
	: m_owner{ owner }
	, m_device{ device }
{
	for (uint32_t i = 0; i < MaxResources; ++i)
	{
		ResetDesc(i);
		ResetData(i);
		ResetHash(i);
		m_freeList.push(i);
	}
}


ResourceHandle RootSignatureFactory::CreateRootSignature(const RootSignatureDesc& rootSignatureDesc)
{
	// Validate RootSignatureDesc
	if (!rootSignatureDesc.Validate())
	{
		LogError(LogVulkan) << "RootSignature is not valid!" << endl;
		return nullptr;
	}

	std::vector<VkDescriptorSetLayout> vkDescriptorSetLayouts;
	std::vector<wil::com_ptr<CVkDescriptorSetLayout>> descriptorSetLayouts;
	std::vector<VkPushConstantRange> vkPushConstantRanges;
	std::unordered_map<uint32_t, std::vector<DescriptorBindingDesc>> layoutBindingMap;
	std::unordered_map<uint32_t, uint32_t> rootParameterIndexToDescriptorSetMap;
	uint32_t pushConstantOffset{ 0 };

	size_t hashCode = Utility::g_hashStart;

	uint32_t rootParamIndex = 0;
	for (const auto& rootParameter : rootSignatureDesc.rootParameters)
	{
		std::vector<VkDescriptorSetLayoutBinding> vkLayoutBindings;
		std::vector<DescriptorBindingDesc> layoutBindingDescs;

		VkShaderStageFlags shaderStageFlags = ShaderStageToVulkan(rootParameter.shaderVisibility);

		bool createLayout = true;
		uint32_t offset = 0;

		if (rootParameter.parameterType == RootParameterType::RootConstants)
		{
			VkPushConstantRange& vkPushConstantRange = vkPushConstantRanges.emplace_back();
			vkPushConstantRange.offset = pushConstantOffset;
			vkPushConstantRange.size = rootParameter.num32BitConstants * 4;
			vkPushConstantRange.stageFlags = shaderStageFlags;
			pushConstantOffset += rootParameter.num32BitConstants * 4;

			hashCode = Utility::HashState(&vkPushConstantRange, 1, hashCode);

			createLayout = false;
		}
		else if (rootParameter.parameterType == RootParameterType::RootCBV ||
			rootParameter.parameterType == RootParameterType::RootSRV ||
			rootParameter.parameterType == RootParameterType::RootUAV)
		{
			VkDescriptorSetLayoutBinding& vkBinding = vkLayoutBindings.emplace_back();
			vkBinding.descriptorType = RootParameterTypeToVulkanDescriptorType(rootParameter.parameterType);

			offset = GetDescriptorOffset(rootSignatureDesc.bindingOffsets, vkBinding.descriptorType);

			vkBinding.binding = rootParameter.startRegister + offset;
			vkBinding.descriptorCount = 1;
			vkBinding.stageFlags = shaderStageFlags;
			vkBinding.pImmutableSamplers = nullptr;

			hashCode = Utility::HashState(&vkBinding, 1, hashCode);

			DescriptorBindingDesc& bindingDesc = layoutBindingDescs.emplace_back();
			bindingDesc.descriptorType = vkBinding.descriptorType;
			bindingDesc.startSlot = rootParameter.startRegister;
			bindingDesc.numDescriptors = vkBinding.descriptorCount;
			bindingDesc.offset = offset;
		}
		else if (rootParameter.parameterType == RootParameterType::Table)
		{
			for (const auto& range : rootParameter.table)
			{
				VkDescriptorSetLayoutBinding& vkBinding = vkLayoutBindings.emplace_back();
				vkBinding.descriptorCount = range.numDescriptors;
				vkBinding.descriptorType = DescriptorTypeToVulkan(range.descriptorType);

				offset = GetDescriptorOffset(rootSignatureDesc.bindingOffsets, vkBinding.descriptorType);

				vkBinding.binding = range.startRegister + offset;
				vkBinding.stageFlags = shaderStageFlags;
				vkBinding.pImmutableSamplers = nullptr;

				hashCode = Utility::HashState(&vkBinding, 1, hashCode);

				DescriptorBindingDesc& bindingDesc = layoutBindingDescs.emplace_back();
				bindingDesc.descriptorType = vkBinding.descriptorType;
				bindingDesc.startSlot = rootParameter.startRegister;
				bindingDesc.numDescriptors = vkBinding.descriptorCount;
				bindingDesc.offset = offset;
			}
		}

		if (createLayout)
		{
			VkDescriptorSetLayoutCreateInfo createInfo{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.bindingCount = (uint32_t)vkLayoutBindings.size(),
				.pBindings = vkLayoutBindings.data()
			};

			VkDescriptorSetLayout vkDescriptorSetLayout{ VK_NULL_HANDLE };
			vkCreateDescriptorSetLayout(*m_device, &createInfo, nullptr, &vkDescriptorSetLayout);
			vkDescriptorSetLayouts.push_back(vkDescriptorSetLayout);

			wil::com_ptr<CVkDescriptorSetLayout> descriptorSetLayout = Create<CVkDescriptorSetLayout>(m_device.get(), vkDescriptorSetLayout);
			descriptorSetLayouts.push_back(descriptorSetLayout);

			layoutBindingMap[rootParamIndex] = layoutBindingDescs;
			rootParameterIndexToDescriptorSetMap[rootParamIndex] = (uint32_t)descriptorSetLayouts.size() - 1;
		}

		++rootParamIndex;
	}

	RootSignatureRecord* record = nullptr;
	ResourceHandle returnHandle;
	bool firstCompile = false;
	{
		lock_guard<mutex> CS(m_mutex);

		auto iter = m_hashToRecordMap.find(hashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == m_hashToRecordMap.end())
		{
			firstCompile = true;

			auto recordPtr = make_unique<RootSignatureRecord>();
			m_hashToRecordMap.emplace(make_pair(hashCode, std::move(recordPtr)));

			record = m_hashToRecordMap[hashCode].get();
		}
		else
		{
			record = iter->second.get();
			returnHandle = record->weakHandle.lock();

			if (!returnHandle)
			{
				returnHandle.reset();
				record->isReady = false;
				firstCompile = true;
			}
		}
	}

	if (firstCompile)
	{
		VkPipelineLayoutCreateInfo createInfo{
			.sType						= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount			= (uint32_t)vkDescriptorSetLayouts.size(),
			.pSetLayouts				= vkDescriptorSetLayouts.data(),
			.pushConstantRangeCount	= (uint32_t)vkPushConstantRanges.size(),
			.pPushConstantRanges		= vkPushConstantRanges.data()
		};

		VkPipelineLayout vkPipelineLayout{ VK_NULL_HANDLE };
		vkCreatePipelineLayout(*m_device, &createInfo, nullptr, &vkPipelineLayout);

		wil::com_ptr<CVkPipelineLayout> pipelineLayout = Create<CVkPipelineLayout>(m_device.get(), vkPipelineLayout);

		assert(!m_freeList.empty());

		uint32_t index = m_freeList.front();
		m_freeList.pop();

		m_descs[index] = rootSignatureDesc;

		RootSignatureData rootSignatureData{
			.pipelineLayout							= pipelineLayout,
			.layoutBindingMap						= layoutBindingMap,
			.rootParameterIndexToDescriptorSetMap	= rootParameterIndexToDescriptorSetMap,
			.descriptorSetLayouts					= descriptorSetLayouts
		};

		m_rootSignatureData[index] = rootSignatureData;
		m_hashList[index] = hashCode;
		
		returnHandle = make_shared<ResourceHandleType>(index, IResourceManager::ManagedRootSignature, m_owner);

		record->weakHandle = returnHandle;
		record->isReady = true;
	}
	else
	{
		while (!record->isReady)
		{
			this_thread::yield();
		}
	}

	assert(returnHandle);
	return returnHandle;
}


void RootSignatureFactory::Destroy(uint32_t index)
{
	std::lock_guard lock(m_mutex);

	size_t hash = m_hashList[index];

	m_hashToRecordMap.erase(hash);

	ResetDesc(index);
	ResetData(index);
	ResetHash(index);
	m_freeList.push(index);
}


const RootSignatureDesc& RootSignatureFactory::GetDesc(uint32_t index) const
{
	return m_descs[index];
}


VkPipelineLayout RootSignatureFactory::GetPipelineLayout(uint32_t index) const
{
	return m_rootSignatureData[index].pipelineLayout->Get();
}


CVkDescriptorSetLayout* RootSignatureFactory::GetDescriptorSetLayout(uint32_t index, uint32_t rootParameterIndex) const
{
	return m_rootSignatureData[index].descriptorSetLayouts[rootParameterIndex].get();
}


const std::vector<DescriptorBindingDesc>& RootSignatureFactory::GetLayoutBindings(uint32_t index, uint32_t paramIndex) const
{
	auto it = m_rootSignatureData[index].layoutBindingMap.find(paramIndex);
	assert(it != m_rootSignatureData[index].layoutBindingMap.end());

	return it->second;
}

} // namespace Luna::VK