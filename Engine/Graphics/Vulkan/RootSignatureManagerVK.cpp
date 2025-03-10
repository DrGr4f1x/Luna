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

#include "RootSignatureManagerVK.h"

#include "DescriptorAllocatorVK.h"
#include "DescriptorSetManagerVK.h"


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


RootSignatureManager* g_rootSignatureManager{ nullptr };


RootSignatureManager::RootSignatureManager(CVkDevice* device)
	: m_device{ device }
{
	assert(g_rootSignatureManager == nullptr);

	// Populate free list and data arrays
	for (uint32_t i = 0; i < MaxItems; ++i)
	{
		m_freeList.push(i);
		m_rootSignatureData[i] = RootSignatureData{};
		m_descs[i] = RootSignatureDesc{};
	}

	g_rootSignatureManager = this;
}


RootSignatureManager::~RootSignatureManager()
{
	g_rootSignatureManager = nullptr;
}


RootSignatureHandle RootSignatureManager::CreateRootSignature(const RootSignatureDesc& rootSignatureDesc)
{
	std::lock_guard guard(m_allocationMutex);

	assert(!m_freeList.empty());

	uint32_t index = m_freeList.front();
	m_freeList.pop();

	m_descs[index] = rootSignatureDesc;

	m_rootSignatureData[index] = FindOrCreateRootSignatureData(rootSignatureDesc);

	return Create<RootSignatureHandleType>(index, this);
}


void RootSignatureManager::DestroyHandle(RootSignatureHandleType* handle)
{
	assert(handle != nullptr);

	std::lock_guard guard(m_allocationMutex);

	// TODO: queue this up to execute in one big batch, e.g. at the end of the frame

	uint32_t index = handle->GetIndex();
	m_descs[index] = RootSignatureDesc{};
	m_rootSignatureData[index] = RootSignatureData{};

	m_freeList.push(index);
}


const RootSignatureDesc& RootSignatureManager::GetDesc(const RootSignatureHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_descs[index];
}


uint32_t RootSignatureManager::GetNumRootParameters(const RootSignatureHandleType* handle) const
{
	return (uint32_t)GetDesc(handle).rootParameters.size();
}


DescriptorSetHandle RootSignatureManager::CreateDescriptorSet(RootSignatureHandleType* handle, uint32_t index) const
{
	const auto& rootSignatureDesc = GetDesc(handle);
	const auto& data = GetData(handle);

	const auto& rootParam = rootSignatureDesc.rootParameters[index];

	const bool isDynamicBuffer = rootParam.parameterType == RootParameterType::RootCBV ||
		rootParam.parameterType == RootParameterType::RootSRV ||
		rootParam.parameterType == RootParameterType::RootUAV;

	DescriptorSetDesc descriptorSetDesc{
		.descriptorSetLayout	= data.descriptorSetLayouts[index].get(),
		.rootParameter			= rootParam,
		.bindingOffsets			= rootSignatureDesc.bindingOffsets,
		.numDescriptors			= rootParam.GetNumDescriptors(),
		.isDynamicBuffer		= isDynamicBuffer
	};

	return GetVulkanDescriptorSetManager()->CreateDescriptorSet(descriptorSetDesc);
}


VkPipelineLayout RootSignatureManager::GetPipelineLayout(RootSignatureHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_rootSignatureData[index].pipelineLayout->Get();
}


CVkDescriptorSetLayout* RootSignatureManager::GetDescriptorSetLayout(RootSignatureHandleType* handle, uint32_t paramIndex) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();

	int descriptorSetIndex = GetDescriptorSetIndexFromRootParameterIndex(handle, paramIndex);

	if (descriptorSetIndex != -1)
	{
		return m_rootSignatureData[index].descriptorSetLayouts[descriptorSetIndex].get();
	}

	return VK_NULL_HANDLE;
}


int RootSignatureManager::GetDescriptorSetIndexFromRootParameterIndex(RootSignatureHandleType* handle, uint32_t paramIndex) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();

	const auto& paramToSetMap = m_rootSignatureData[index].rootParameterIndexToDescriptorSetMap;
	auto it = paramToSetMap.find(paramIndex);
	if (it != paramToSetMap.end())
	{
		return it->second;
	}

	return -1;
}


const std::vector<DescriptorBindingDesc>& RootSignatureManager::GetLayoutBindings(RootSignatureHandleType* handle, uint32_t paramIndex) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();

	auto it = m_rootSignatureData[index].layoutBindingMap.find(paramIndex);
	assert(it != m_rootSignatureData[index].layoutBindingMap.end());

	return it->second;
}


const RootSignatureData& RootSignatureManager::GetData(RootSignatureHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_rootSignatureData[index];
}


RootSignatureData RootSignatureManager::FindOrCreateRootSignatureData(const RootSignatureDesc& rootSignatureDesc)
{
	// Validate RootSignatureDesc
	if (!rootSignatureDesc.Validate())
	{
		LogError(LogVulkan) << "RootSignature is not valid!" << endl;
		return RootSignatureData{};
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
				.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.bindingCount	= (uint32_t)vkLayoutBindings.size(),
				.pBindings		= vkLayoutBindings.data()
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

	CVkPipelineLayout** ppPipelineLayout;
	bool firstCompile = false;
	{
		lock_guard<mutex> CS(m_pipelineLayoutMutex);

		auto iter = m_pipelineLayoutHashMap.find(hashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == m_pipelineLayoutHashMap.end())
		{
			ppPipelineLayout = m_pipelineLayoutHashMap[hashCode].addressof();
			firstCompile = true;
		}
		else
		{
			ppPipelineLayout = &iter->second;
		}
	}

	if (firstCompile)
	{
		VkPipelineLayoutCreateInfo createInfo{
			.sType						= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount				= (uint32_t)vkDescriptorSetLayouts.size(),
			.pSetLayouts				= vkDescriptorSetLayouts.data(),
			.pushConstantRangeCount		= (uint32_t)vkPushConstantRanges.size(),
			.pPushConstantRanges		= vkPushConstantRanges.data()
		};

		VkPipelineLayout vkPipelineLayout{ VK_NULL_HANDLE };
		vkCreatePipelineLayout(*m_device, &createInfo, nullptr, &vkPipelineLayout);

		wil::com_ptr<CVkPipelineLayout> pipelineLayout = Create<CVkPipelineLayout>(m_device.get(), vkPipelineLayout);

		*ppPipelineLayout = pipelineLayout.get();

		(*ppPipelineLayout)->AddRef();
	}

	RootSignatureData rootSignatureData{
		.pipelineLayout							= *ppPipelineLayout,
		.layoutBindingMap						= layoutBindingMap,
		.rootParameterIndexToDescriptorSetMap	= rootParameterIndexToDescriptorSetMap,
		.descriptorSetLayouts					= descriptorSetLayouts
	};

	return rootSignatureData;
}


RootSignatureManager* const GetVulkanRootSignatureManager()
{
	return g_rootSignatureManager;
}

} // namespace Luna::VK