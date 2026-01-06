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

#include "CommandContextVK.h"
#include "DescriptorVK.h"
#include "DeviceManagerVK.h"
#include "DeviceVK.h"

#if USE_LEGACY_DESCRIPTOR_SETS
#include "DescriptorPoolVK.h"
#endif // USE_LEGACY_DESCRIPTOR_SETS

#include "GpuBufferVK.h"
#include "RootSignatureVK.h"
#include "SamplerVK.h"

using namespace std;


namespace Luna::VK
{

#if USE_DESCRIPTOR_BUFFERS
std::mutex DynamicDescriptorBuffer::sm_mutex;
std::vector<wil::com_ptr<CVkBuffer>> DynamicDescriptorBuffer::sm_descriptorBufferPool[2];
std::queue<std::pair<uint64_t, CVkBuffer*>> DynamicDescriptorBuffer::sm_retiredDescriptorBuffers[2];
std::queue<CVkBuffer*> DynamicDescriptorBuffer::sm_availableDescriptorBuffers[2];


DynamicDescriptorBuffer::DynamicDescriptorBuffer(CommandContextVK& owningContext, DescriptorBufferType bufferType)
	: m_owningContext{ owningContext }
	, m_bufferType{ bufferType }
{
	m_offsetAlignment = GetVulkanDevice()->GetDeviceCaps().descriptorBuffer.descriptorBufferOffsetAlignment;
	m_graphicsCache.Clear();
	m_computeCache.Clear();
}


void DynamicDescriptorBuffer::DestroyAll()
{
	sm_descriptorBufferPool[0].clear();
	sm_descriptorBufferPool[1].clear();
}


void DynamicDescriptorBuffer::CleanupUsedBuffers(uint64_t fenceValue)
{
	RetireCurrentBuffer();
	RetireUsedBuffers(fenceValue);
	m_graphicsCache.Clear();
	m_computeCache.Clear();
}


void DynamicDescriptorBuffer::ParseRootSignature(const RootSignature& rootSignature, bool graphicsPipe)
{
	auto& descriptorCache = graphicsPipe ? m_graphicsCache : m_computeCache;

	size_t requiredSize = 0;
	const bool isSamplerBuffer = m_bufferType == DescriptorBufferType::Sampler;

	if (isSamplerBuffer)
	{
		requiredSize = rootSignature.GetSamplerDescriptorSetLayoutSize();
	}
	else
	{
		requiredSize = rootSignature.GetResourceDescriptorSetLayoutSize();
	}

	if (!HasSpace(requiredSize))
	{
		RetireCurrentBuffer();

		if (m_currentBufferPtr == nullptr)
		{
			assert(m_currentOffset == 0);
			m_currentBufferPtr = RequestDescriptorBuffer(m_bufferType);
			ThrowIfFailed(vmaMapMemory(m_currentBufferPtr->GetAllocator(), m_currentBufferPtr->GetAllocation(), (void**)&m_bufferStart));
			m_freeSpace = GetBufferSize(m_bufferType);
		}
	}

	descriptorCache.Clear();

	descriptorCache.pipelineLayout = rootSignature.GetPipelineLayout();

	size_t allocatedSize = 0;
	const uint32_t numRootParameters = rootSignature.GetNumRootParameters();
	for (uint32_t i = 0; i < numRootParameters; ++i)
	{
		const auto& rootParameter = rootSignature.GetRootParameter(i);

		if (rootParameter.parameterType == RootParameterType::Table)
		{
			if (isSamplerBuffer == rootParameter.IsSamplerTable())
			{
				descriptorCache.descriptorSetLayouts[i] = rootSignature.GetDescriptorSetLayout(i);

				const size_t descriptorSetLayoutSize = descriptorCache.descriptorSetLayouts[i]->GetDescriptorSetSize();
				descriptorCache.tableAllocations[i] = Allocate(descriptorSetLayoutSize);

				allocatedSize += descriptorSetLayoutSize;
			}
		}
	}

	assert(allocatedSize == requiredSize);
}


void DynamicDescriptorBuffer::UpdateAndBindDescriptorSets(VkCommandBuffer commandBuffer, bool graphicsPipe)
{
	auto& descriptorCache = graphicsPipe ? m_graphicsCache : m_computeCache;
	
	// Bind the descriptor buffer
	m_owningContext.SetDescriptorBuffer(m_bufferType, m_currentBufferPtr->Get());

	// Bind the buffer offsets
	VkPipelineLayout pipelineLayout = descriptorCache.pipelineLayout;
	uint32_t bufferIndex = m_bufferType == DescriptorBufferType::Resource ? 0 : 1;

	VkDeviceSize localOffset = 0;
	for (uint32_t rootIndex = 0; rootIndex < MaxRootParameters; ++rootIndex)
	{
		if (descriptorCache.tableAllocations[rootIndex].mem != nullptr)
		{
			VkDeviceSize offset = descriptorCache.tableAllocations[rootIndex].offset;

			vkCmdSetDescriptorBufferOffsetsEXT(
				commandBuffer,
				graphicsPipe ? VK_PIPELINE_BIND_POINT_GRAPHICS : VK_PIPELINE_BIND_POINT_COMPUTE,
				pipelineLayout,
				rootIndex,
				1,
				&bufferIndex,
				&offset
			);
		}
	}
}


void DynamicDescriptorBuffer::SetSRV(CommandListType type, uint32_t rootIndex, uint32_t srvRegister, uint32_t arrayIndex, const IColorBuffer* colorBuffer)
{
	auto& descriptorCache = (type == CommandListType::Graphics) ? m_graphicsCache : m_computeCache;

	srvRegister += GetRegisterShiftSRV();
	descriptorCache.SetDescriptor(rootIndex, srvRegister, arrayIndex, (const Descriptor*)colorBuffer->GetSrvDescriptor());
}


void DynamicDescriptorBuffer::SetSRV(CommandListType type, uint32_t rootIndex, uint32_t srvRegister, uint32_t arrayIndex, const IDepthBuffer* depthBuffer, bool depthSrv)
{
	auto& descriptorCache = (type == CommandListType::Graphics) ? m_graphicsCache : m_computeCache;

	srvRegister += GetRegisterShiftSRV();
	descriptorCache.SetDescriptor(rootIndex, srvRegister, arrayIndex, (const Descriptor*)depthBuffer->GetSrvDescriptor(depthSrv));
}


void DynamicDescriptorBuffer::SetSRV(CommandListType type, uint32_t rootIndex, uint32_t srvRegister, uint32_t arrayIndex, const IGpuBuffer* gpuBuffer)
{
	auto& descriptorCache = (type == CommandListType::Graphics) ? m_graphicsCache : m_computeCache;

	srvRegister += GetRegisterShiftSRV();
	descriptorCache.SetDescriptor(rootIndex, srvRegister, arrayIndex, (const Descriptor*)gpuBuffer->GetSrvDescriptor());
}


void DynamicDescriptorBuffer::SetSRV(CommandListType type, uint32_t rootIndex, uint32_t srvRegister, uint32_t arrayIndex, const ITexture* texture)
{
	auto& descriptorCache = (type == CommandListType::Graphics) ? m_graphicsCache : m_computeCache;

	srvRegister += GetRegisterShiftSRV();
	descriptorCache.SetDescriptor(rootIndex, srvRegister, arrayIndex, (const Descriptor*)texture->GetDescriptor());
}


void DynamicDescriptorBuffer::SetUAV(CommandListType type, uint32_t rootIndex, uint32_t uavRegister, uint32_t arrayIndex, const IColorBuffer* colorBuffer)
{
	auto& descriptorCache = (type == CommandListType::Graphics) ? m_graphicsCache : m_computeCache;

	uavRegister += GetRegisterShiftUAV();
	descriptorCache.SetDescriptor(rootIndex, uavRegister, arrayIndex, (const Descriptor*)colorBuffer->GetUavDescriptor());
}


void DynamicDescriptorBuffer::SetUAV(CommandListType type, uint32_t rootIndex, uint32_t uavRegister, uint32_t arrayIndex, const IDepthBuffer* depthBuffer)
{
	auto& descriptorCache = (type == CommandListType::Graphics) ? m_graphicsCache : m_computeCache;
	assert_msg(false, "Depth UAVs not yet supported");
}


void DynamicDescriptorBuffer::SetUAV(CommandListType type, uint32_t rootIndex, uint32_t uavRegister, uint32_t arrayIndex, const IGpuBuffer* gpuBuffer)
{
	auto& descriptorCache = (type == CommandListType::Graphics) ? m_graphicsCache : m_computeCache;

	uavRegister += GetRegisterShiftUAV();
	descriptorCache.SetDescriptor(rootIndex, uavRegister, arrayIndex, (const Descriptor*)gpuBuffer->GetUavDescriptor());
}


void DynamicDescriptorBuffer::SetCBV(CommandListType type, uint32_t rootIndex, uint32_t cbvRegister, uint32_t arrayIndex, const IGpuBuffer* gpuBuffer)
{
	auto& descriptorCache = (type == CommandListType::Graphics) ? m_graphicsCache : m_computeCache;

	cbvRegister += GetRegisterShiftCBV();
	descriptorCache.SetDescriptor(rootIndex, cbvRegister, arrayIndex, (const Descriptor*)gpuBuffer->GetCbvDescriptor());
}


void DynamicDescriptorBuffer::SetSampler(CommandListType type, uint32_t rootIndex, uint32_t samplerRegister, uint32_t arrayIndex, const ISampler* sampler)
{
	auto& descriptorCache = (type == CommandListType::Graphics) ? m_graphicsCache : m_computeCache;

	samplerRegister += GetRegisterShiftSampler();
	descriptorCache.SetDescriptor(rootIndex, samplerRegister, arrayIndex, (const Descriptor*)sampler->GetDescriptor());
}


/* static */ CVkBuffer* DynamicDescriptorBuffer::RequestDescriptorBuffer(DescriptorBufferType bufferType)
{
	lock_guard<mutex> CS(sm_mutex);

	uint32_t idx = bufferType == DescriptorBufferType::Sampler ? 1 : 0;

	while (!sm_retiredDescriptorBuffers[idx].empty() && GetVulkanDeviceManager()->IsFenceComplete(sm_retiredDescriptorBuffers[idx].front().first))
	{
		sm_availableDescriptorBuffers[idx].push(sm_retiredDescriptorBuffers[idx].front().second);
		sm_retiredDescriptorBuffers[idx].pop();
	}

	if (!sm_availableDescriptorBuffers[idx].empty())
	{
		CVkBuffer* bufferPtr = sm_availableDescriptorBuffers[idx].front();
		sm_availableDescriptorBuffers[idx].pop();
		return bufferPtr;
	}
	else
	{
		auto device = GetVulkanDevice();
		auto newBuffer = device->CreateDescriptorBuffer(bufferType, GetBufferSize(bufferType));
		sm_descriptorBufferPool[idx].emplace_back(newBuffer);
		return newBuffer.get();
	}
}


/* static */ void DynamicDescriptorBuffer::DiscardDescriptorBuffers(DescriptorBufferType bufferType, uint64_t fenceValue, const vector<CVkBuffer*>& usedBuffers)
{
	const uint32_t idx = bufferType == DescriptorBufferType::Sampler ? 1 : 0;

	lock_guard<mutex> CS(sm_mutex);

	for (auto iter = usedBuffers.begin(); iter != usedBuffers.end(); ++iter)
	{
		sm_retiredDescriptorBuffers[idx].push(make_pair(fenceValue, *iter));
	}
}


void DynamicDescriptorBuffer::RetireCurrentBuffer()
{
	if (m_currentBufferPtr != nullptr)
	{
		vmaUnmapMemory(m_currentBufferPtr->GetAllocator(), m_currentBufferPtr->GetAllocation());
		m_retiredBuffers.push_back(m_currentBufferPtr);
		m_currentBufferPtr = nullptr;
	}
	m_currentOffset = 0;
	m_bufferStart = nullptr;
}


void DynamicDescriptorBuffer::RetireUsedBuffers(uint64_t fenceValue)
{
	DiscardDescriptorBuffers(m_bufferType, fenceValue, m_retiredBuffers);
	m_retiredBuffers.clear();
}


DescriptorBufferAllocation DynamicDescriptorBuffer::Allocate(size_t sizeInBytes)
{
	size_t alignedSize = Math::AlignUp(sizeInBytes, m_offsetAlignment);
	assert(m_freeSpace >= alignedSize);

	std::byte* allocation = m_bufferStart + m_currentOffset;
	m_currentOffset += alignedSize;

	m_freeSpace -= alignedSize;

	return DescriptorBufferAllocation{
		.mem		= allocation,
		.offset		= (size_t)(allocation - m_bufferStart)
	};
}


size_t DynamicDescriptorBuffer::GetBufferSize(DescriptorBufferType bufferType)
{
	auto device = GetVulkanDevice();

	const size_t bufferSize = (bufferType == DescriptorBufferType::Resource) ?
		device->GetDeviceCaps().descriptorBuffer.descriptorSize.largest :
		device->GetDeviceCaps().descriptorBuffer.descriptorSize.sampler;

	const size_t numDescriptors = (bufferType == DescriptorBufferType::Resource) ?
		sm_numResourceDescriptors :
		sm_numSamplerDescriptors;

	return bufferSize * numDescriptors;
}


void DynamicDescriptorBuffer::DescriptorCache::SetDescriptor(uint32_t rootIndex, uint32_t descriptorRegister, uint32_t arrayIndex, const Descriptor* descriptor)
{
	auto device = GetVulkanDevice();

	assert(descriptorSetLayouts[rootIndex] != nullptr);
	assert(tableAllocations[rootIndex].mem != nullptr);

	size_t offset = descriptorSetLayouts[rootIndex]->GetBindingOffset(descriptorRegister) + arrayIndex * descriptor->GetRawDescriptorSize();

	descriptor->CopyRawDescriptor((void*)(tableAllocations[rootIndex].mem + offset));
}


void DynamicDescriptorBuffer::DescriptorCache::Clear()
{
	for (uint32_t i = 0; i < MaxRootParameters; ++i)
	{
		tableAllocations[i].mem = nullptr;
		tableAllocations[i].offset = 0;
		descriptorSetLayouts[i] = nullptr;
	}
	pipelineLayout = VK_NULL_HANDLE;
}

#endif // USE_DESCRIPTOR_BUFFERS


#if USE_LEGACY_DESCRIPTOR_SETS
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

		/*const auto& bindings = rootSignature.GetLayoutBindings(rootParamIndex);
		for (const auto& binding : bindings)
		{
			for (uint32_t bindingIndex = 0; bindingIndex < binding.numDescriptors; ++bindingIndex)
			{
				descriptorCache.SetDescriptorBinding(bindingIndex + binding.startSlot, binding.descriptorType, binding.offset);
			}
		}*/
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
#endif // USE_LEGACY_DESCRIPTOR_SETS

} // namespace Luna::VK