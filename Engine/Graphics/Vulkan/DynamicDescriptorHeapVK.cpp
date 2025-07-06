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

#include "DynamicDescriptorHeapVK.h"

#include "DescriptorPoolVK.h"
#include "RootSignatureVK.h"

using namespace std;


namespace Luna::VK
{

DefaultDynamicDescriptorHeap::DefaultDynamicDescriptorHeap(CVkDevice* device)
	: m_device{ device }
{
	Reset();

	// Set the device on the graphics descriptor caches
	for (uint32_t i = 0; i < (uint32_t)m_descriptorCaches[0].size(); ++i)
	{
		m_descriptorCaches[0][i].device = device->Get();
		m_descriptorCaches[0][i].rootParameterIndex = i;
	}

	// Set the device on the compute descriptor caches
	for (uint32_t i = 0; i < (uint32_t)m_descriptorCaches[1].size(); ++i)
	{
		m_descriptorCaches[1][i].device = device->Get();
		m_descriptorCaches[1][i].rootParameterIndex = i;
	}
}


void DefaultDynamicDescriptorHeap::SetDescriptorImageInfo(uint32_t rootParameter, uint32_t offset, VkDescriptorImageInfo descriptorImageInfo, bool graphicsPipe)
{
	const uint32_t pipeIndex = graphicsPipe ? 0 : 1;
	m_descriptorCaches[pipeIndex][rootParameter].SetDescriptorImageInfo(offset, descriptorImageInfo);
}


void DefaultDynamicDescriptorHeap::SetDescriptorBufferInfo(uint32_t rootParameter, uint32_t offset, VkDescriptorBufferInfo descriptorBufferInfo, bool graphicsPipe)
{
	const uint32_t pipeIndex = graphicsPipe ? 0 : 1;
	m_descriptorCaches[pipeIndex][rootParameter].SetDescriptorBufferInfo(offset, descriptorBufferInfo);
}


void DefaultDynamicDescriptorHeap::SetDescriptorBufferView(uint32_t rootParameter, uint32_t offset, VkBufferView descriptorBufferView, bool graphicsPipe)
{
	const uint32_t pipeIndex = graphicsPipe ? 0 : 1;
	m_descriptorCaches[pipeIndex][rootParameter].SetDescriptorBufferView(offset, descriptorBufferView);
}


void DefaultDynamicDescriptorHeap::CleanupUsedPools(uint64_t fenceValue)
{
	for (auto& [layout, pool] : m_layoutToPoolMap)
	{
		pool->RetirePool(fenceValue);
	}
}


void DefaultDynamicDescriptorHeap::ParseRootSignature(const RootSignature& rootSignature, bool graphicsPipe)
{
	ScopedEvent event("ParseRootSignature");

	Reset();

	const uint32_t pipeIndex = graphicsPipe ? 0 : 1;
	const auto& rootSignatureDesc = rootSignature.GetDesc();

	// Get the pipeline layout
	m_pipelineLayout[pipeIndex] = rootSignature.GetPipelineLayout();

	// Get the descriptor set layout for each root parameter and, if valid, set a bit in the
	// m_activeDescriptorSetMap.  The only root parameter type that does not have a descriptor set layout
	// is RootConstants (aka push constants).
	const uint32_t numParams = rootSignature.GetNumRootParameters();
	for (uint32_t rootParamIndex = 0; rootParamIndex < numParams; ++rootParamIndex)
	{
		// Get layout
		auto layout = rootSignature.GetDescriptorSetLayout(rootParamIndex);

		// Allocate descriptor set
		auto& descriptorCache = m_descriptorCaches[pipeIndex][rootParamIndex];
		const auto& rootParameter = rootSignatureDesc.rootParameters[rootParamIndex];
		auto pool = FindOrCreateDescriptorPoolCache(layout, rootParameter);
		descriptorCache.descriptorSet = pool->AllocateDescriptorSet();

		// Process descriptor bindings
		ScopedEvent event("Process descriptor bindings");

		const auto& bindings = rootSignature.GetLayoutBindings(rootParamIndex);
		for (const auto& binding : bindings)
		{
			for (uint32_t bindingIndex = 0; bindingIndex < binding.numDescriptors; ++bindingIndex)
			{
				descriptorCache.SetDescriptorBinding(bindingIndex + binding.startSlot, binding.descriptorType, binding.offset);
			}
		}
	}
}


void DefaultDynamicDescriptorHeap::UpdateAndBindDescriptorSets(VkCommandBuffer commandBuffer, bool graphicsPipe)
{
	const VkPipelineBindPoint bindPoint = graphicsPipe ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE;
	const uint32_t pipeIndex = graphicsPipe ? 0 : 1;

	VkPipelineLayout pipelineLayout = m_pipelineLayout[pipeIndex];
	assert(pipelineLayout != VK_NULL_HANDLE);

	for (uint32_t rootParameterIndex = 0; rootParameterIndex < MaxRootParameters; ++rootParameterIndex)
	{
		auto& descriptorCache = m_descriptorCaches[pipeIndex][rootParameterIndex];

		if (descriptorCache.UpdateDescriptorSet())
		{
			uint32_t dynamicOffset = descriptorCache.GetDynamicOffset();

			vkCmdBindDescriptorSets(
				commandBuffer,
				bindPoint,
				pipelineLayout,
				rootParameterIndex,
				1,
				&descriptorCache.descriptorSet,
				descriptorCache.HasDynamicOffset() ? 1 : 0,
				descriptorCache.HasDynamicOffset() ? &dynamicOffset : nullptr);
		}
	}
}


DescriptorPoolCache* DefaultDynamicDescriptorHeap::FindOrCreateDescriptorPoolCache(CVkDescriptorSetLayout* layout, const RootParameter& rootParameter)
{
	ScopedEvent event("FindOrCreateDescriptorPoolCache");

	auto it = m_layoutToPoolMap.find(layout->Get());
	if (it != m_layoutToPoolMap.end())
	{
		return it->second.get();
	}

	DescriptorPoolDesc descriptorPoolDesc{
		.device = m_device.get(),
		.layout = layout,
		.rootParameter = rootParameter
	};

	auto pool = make_shared<DescriptorPoolCache>(descriptorPoolDesc);
	m_layoutToPoolMap[layout->Get()] = pool;

	return pool.get();
}


void DefaultDynamicDescriptorHeap::Reset()
{
	m_pipelineLayout[0] = VK_NULL_HANDLE;
	m_pipelineLayout[1] = VK_NULL_HANDLE;

	// Reset graphics descriptor caches
	for (uint32_t i = 0; i < m_descriptorCaches[0].size(); ++i)
	{
		m_descriptorCaches[0][i].Reset();
	}

	// Reset compute descriptor caches
	for (uint32_t i = 0; i < m_descriptorCaches[1].size(); ++i)
	{
		m_descriptorCaches[1][i].Reset();
	}
}


void DefaultDynamicDescriptorHeap::DescriptorCache::SetDescriptorImageInfo(uint32_t offset, VkDescriptorImageInfo descriptorImageInfo)
{
	assert(descriptorBindings.find(offset) != descriptorBindings.end());

	auto& descriptorBinding = descriptorBindings[offset];

	assert(IsDescriptorImageInfoType(descriptorBinding.descriptorType));

	descriptorBinding.descriptorInfo = descriptorImageInfo;
	dirty = true;
}


void DefaultDynamicDescriptorHeap::DescriptorCache::SetDescriptorBufferInfo(uint32_t offset, VkDescriptorBufferInfo descriptorBufferInfo)
{
	assert(descriptorBindings.find(offset) != descriptorBindings.end());

	auto& descriptorBinding = descriptorBindings[offset];

	assert(IsDescriptorBufferInfoType(descriptorBinding.descriptorType));

	descriptorBinding.descriptorInfo = descriptorBufferInfo;
	dirty = true;
}


void DefaultDynamicDescriptorHeap::DescriptorCache::SetDescriptorBufferView(uint32_t offset, VkBufferView descriptorBufferView)
{
	assert(descriptorBindings.find(offset) != descriptorBindings.end());

	auto& descriptorBinding = descriptorBindings[offset];

	assert(IsDescriptorBufferViewType(descriptorBinding.descriptorType));

	descriptorBinding.descriptorInfo = descriptorBufferView;
	dirty = true;
}


void DefaultDynamicDescriptorHeap::DescriptorCache::Reset()
{
	descriptorSet = VK_NULL_HANDLE;
	descriptorBindings.clear();
	writeDescriptorSets.clear();
	hasDynamicOffset = false;
	dynamicOffset = 0;
}


bool DefaultDynamicDescriptorHeap::DescriptorCache::UpdateDescriptorSet()
{
	if (!dirty)
	{
		return false;
	}

	assert(descriptorSet != VK_NULL_HANDLE);

	for (const auto& [bindingSlot, descriptorBinding] : descriptorBindings)
	{
		VkWriteDescriptorSet writeSet{
			.sType				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext				= nullptr,
			.dstSet				= descriptorSet,
			.dstBinding			= bindingSlot + descriptorBinding.offset,
			.dstArrayElement	= 0,
			.descriptorCount	= 1,
			.descriptorType		= descriptorBinding.descriptorType
		};

		bool didSetData = false;

		switch (descriptorBinding.descriptorType)
		{
		case VK_DESCRIPTOR_TYPE_SAMPLER:
		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			writeSet.pImageInfo = std::get_if<VkDescriptorImageInfo>(&descriptorBinding.descriptorInfo);
			didSetData = true;
			break;
		
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
			writeSet.pBufferInfo = std::get_if<VkDescriptorBufferInfo>(&descriptorBinding.descriptorInfo);
			didSetData = true;
			break;

		case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
		case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
			writeSet.pTexelBufferView = std::get_if<VkBufferView>(&descriptorBinding.descriptorInfo);
			didSetData = true;
			break;
		}

		if (didSetData)
		{
			writeDescriptorSets.push_back(writeSet);
		}
	}

	assert(device != VK_NULL_HANDLE);
	vkUpdateDescriptorSets(
		device,
		(uint32_t)writeDescriptorSets.size(),
		writeDescriptorSets.data(),
		0,
		nullptr);

	return true;
}


void DefaultDynamicDescriptorHeap::DescriptorCache::SetDescriptorBinding(uint32_t bindingSlot, VkDescriptorType descriptorType, uint32_t offset)
{
	DescriptorBinding binding{ .descriptorType = descriptorType, .offset = offset };
	descriptorBindings[bindingSlot] = binding;
	if (descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC || descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
	{
		hasDynamicOffset = true;
	}
}

} // namespace Luna::VK