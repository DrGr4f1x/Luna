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
#include "GpuBufferVK.h"
#include "RootSignatureVK.h"
#include "SamplerVK.h"

using namespace std;


namespace Luna::VK
{

std::mutex DynamicDescriptorBuffer::sm_mutex;
std::vector<wil::com_ptr<CVkBuffer>> DynamicDescriptorBuffer::sm_descriptorBufferPool[2];
std::queue<std::pair<uint64_t, CVkBuffer*>> DynamicDescriptorBuffer::sm_retiredDescriptorBuffers[2];
std::queue<CVkBuffer*> DynamicDescriptorBuffer::sm_availableDescriptorBuffers[2];


static size_t GetDescriptorSize(DescriptorHeapType bufferType)
{
	auto device = GetVulkanDevice();

	return (bufferType == DescriptorHeapType::Resource) ?
		device->GetDeviceCaps().descriptorBuffer.descriptorSize.largest :
		device->GetDeviceCaps().descriptorBuffer.descriptorSize.sampler;
}


DynamicDescriptorBuffer::DynamicDescriptorBuffer(CommandContextVK& owningContext, DescriptorHeapType bufferType)
	: m_owningContext{ owningContext }
	, m_bufferType{ bufferType }
{
	m_offsetAlignment = GetVulkanDevice()->GetDeviceCaps().descriptorBuffer.descriptorBufferOffsetAlignment;

	const size_t descriptorSize = GetDescriptorSize(bufferType);
	const size_t descriptorCacheMemorySize = sm_numCachedDescriptors * descriptorSize;

	m_graphicsCache.descriptorMemory = (std::byte*)_aligned_malloc(descriptorCacheMemorySize, m_offsetAlignment);
	m_computeCache.descriptorMemory = (std::byte*)_aligned_malloc(descriptorCacheMemorySize, m_offsetAlignment);

	m_graphicsCache.allocatedSize = descriptorCacheMemorySize;
	m_computeCache.allocatedSize = descriptorCacheMemorySize;

	ZeroMemory(m_graphicsCache.descriptorMemory, descriptorCacheMemorySize);
	ZeroMemory(m_computeCache.descriptorMemory, descriptorCacheMemorySize);

	m_graphicsCache.Clear();
	m_computeCache.Clear();
}


DynamicDescriptorBuffer::~DynamicDescriptorBuffer()
{
	_aligned_free(m_graphicsCache.descriptorMemory);
	_aligned_free(m_computeCache.descriptorMemory);
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

	m_rootSignatureSize = 0;
	const bool isSamplerBuffer = m_bufferType == DescriptorHeapType::Sampler;

	if (isSamplerBuffer)
	{
		m_rootSignatureSize = rootSignature.GetSamplerDescriptorSetLayoutSize();
	}
	else
	{
		m_rootSignatureSize = rootSignature.GetResourceDescriptorSetLayoutSize();
	}

	descriptorCache.Clear();

	descriptorCache.pipelineLayout = rootSignature.GetPipelineLayout();

	const uint32_t numRootParameters = rootSignature.GetNumRootParameters();
	uint8_t activeTable = 0;
	for (uint32_t i = 0; i < numRootParameters; ++i)
	{
		const auto& rootParameter = rootSignature.GetRootParameter(i);

		if (rootParameter.parameterType == RootParameterType::Table)
		{
			if (isSamplerBuffer == rootParameter.IsSamplerTable())
			{
				descriptorCache.descriptorSetLayouts[i] = rootSignature.GetDescriptorSetLayout(i);

				const size_t descriptorSetLayoutSize = descriptorCache.descriptorSetLayouts[i]->GetDescriptorSetSize();
				descriptorCache.Allocate(i, descriptorSetLayoutSize, m_offsetAlignment);

				descriptorCache.tableSizes[i] = descriptorSetLayoutSize;
				descriptorCache.totalSize += descriptorSetLayoutSize;

				descriptorCache.activeTables[activeTable] = i;
				descriptorCache.numActiveTables++;
				++activeTable;
			}
		}
	}

	assert(descriptorCache.totalSize == m_rootSignatureSize);
}


void DynamicDescriptorBuffer::UpdateAndBindDescriptorSets(VkCommandBuffer commandBuffer, bool graphicsPipe)
{
	auto& descriptorCache = graphicsPipe ? m_graphicsCache : m_computeCache;
	
	const size_t dirtySize = descriptorCache.GetDirtySize();
	bool acquiredNewBuffer = false;

	// If we don't have enough space to hold the dirty descriptors, acquire a new buffer
	if (!HasSpace(dirtySize))
	{
		RetireCurrentBuffer();

		if (m_currentBufferPtr == nullptr)
		{
			assert(m_currentOffset == 0);
			m_currentBufferPtr = RequestDescriptorBuffer(m_bufferType);
			ThrowIfFailed(vmaMapMemory(m_currentBufferPtr->GetAllocator(), m_currentBufferPtr->GetAllocation(), (void**)&m_bufferStart));
			m_freeSpace = GetBufferSize(m_bufferType);
		}

		acquiredNewBuffer = true;

		// We'll have to copy the whole descriptor cache into the new buffer, 
		// so assert that we have enough space.
		assert(HasSpace(descriptorCache.totalSize));
	}

	// Copy descriptors into buffer
	bool dirtyDescriptorsOnly = !acquiredNewBuffer;
	CopyDescriptors(descriptorCache, dirtyDescriptorsOnly);

	// Bind the descriptor buffer
	m_owningContext.SetDescriptorBuffer(m_bufferType, m_currentBufferPtr->Get());

	// Bind the buffer offsets
	BindDescriptorSetOffsets(commandBuffer, descriptorCache, dirtyDescriptorsOnly, graphicsPipe);
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


/* static */ CVkBuffer* DynamicDescriptorBuffer::RequestDescriptorBuffer(DescriptorHeapType bufferType)
{
	lock_guard<mutex> CS(sm_mutex);

	uint32_t idx = bufferType == DescriptorHeapType::Sampler ? 1 : 0;

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


/* static */ void DynamicDescriptorBuffer::DiscardDescriptorBuffers(DescriptorHeapType bufferType, uint64_t fenceValue, const vector<CVkBuffer*>& usedBuffers)
{
	const uint32_t idx = bufferType == DescriptorHeapType::Sampler ? 1 : 0;

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


DescriptorAllocation DynamicDescriptorBuffer::Allocate(size_t sizeInBytes)
{
	size_t alignedSize = Math::AlignUp(sizeInBytes, m_offsetAlignment);
	assert(m_freeSpace >= alignedSize);

	std::byte* allocation = m_bufferStart + m_currentOffset;
	m_currentOffset += alignedSize;

	m_freeSpace -= alignedSize;

	return DescriptorAllocation{
		.mem		= allocation,
		.offset		= (size_t)(allocation - m_bufferStart)
	};
}


size_t DynamicDescriptorBuffer::GetBufferSize(DescriptorHeapType bufferType)
{
	auto device = GetVulkanDevice();

	const size_t descriptorSize = GetDescriptorSize(bufferType);

	const size_t numDescriptors = (bufferType == DescriptorHeapType::Resource) ?
		sm_numResourceDescriptors :
		sm_numSamplerDescriptors;

	return descriptorSize * numDescriptors;
}


void DynamicDescriptorBuffer::DescriptorCache::SetDescriptor(uint32_t rootIndex, uint32_t descriptorRegister, uint32_t arrayIndex, const Descriptor* descriptor)
{
	auto device = GetVulkanDevice();

	assert(descriptorSetLayouts[rootIndex] != nullptr);
	assert(tableAllocations[rootIndex].mem != nullptr);

	size_t offset = descriptorSetLayouts[rootIndex]->GetBindingOffset(descriptorRegister) + arrayIndex * descriptor->GetRawDescriptorSize();

	descriptor->CopyRawDescriptor((void*)(tableAllocations[rootIndex].mem + offset));

	hasActiveData[rootIndex] = true;
	hasDirtyData[rootIndex] = true;
}


void DynamicDescriptorBuffer::DescriptorCache::Allocate(uint32_t rootIndex, size_t requiredSize, size_t offsetAlignment)
{
	size_t alignedSize = Math::AlignUp(requiredSize, offsetAlignment);
	assert(freeSpace >= alignedSize);

	std::byte* allocation = descriptorMemory + currentOffset;
	currentOffset += alignedSize;

	freeSpace -= alignedSize;

	tableAllocations[rootIndex] = DescriptorAllocation{
		.mem		= allocation,
		.offset		= (size_t)(allocation - descriptorMemory)
	};
}


void DynamicDescriptorBuffer::DescriptorCache::Clear()
{
	for (uint32_t i = 0; i < MaxRootParameters; ++i)
	{
		tableAllocations[i].mem = nullptr;
		tableAllocations[i].offset = 0;

		hasActiveData[i] = false;
		hasDirtyData[i] = false;

		bindingOffsets[i] = (size_t)-1;

		tableSizes[i] = 0;

		descriptorSetLayouts[i] = nullptr;
		activeTables[i] = ~((uint8_t)0);
	}
	totalSize = 0;
	currentOffset = 0;
	freeSpace = allocatedSize;
	pipelineLayout = VK_NULL_HANDLE;
	numActiveTables = 0;
}


size_t DynamicDescriptorBuffer::DescriptorCache::GetDirtySize() const
{
	size_t dirtySize = 0;
	for (uint8_t i = 0; i < numActiveTables; ++i)
	{
		const uint8_t rootIndex = activeTables[i];
		if (hasDirtyData[rootIndex])
		{
			dirtySize += tableSizes[rootIndex];
		}
	}
	return dirtySize;
}


void DynamicDescriptorBuffer::CopyDescriptors(DynamicDescriptorBuffer::DescriptorCache& descriptorCache, bool copyDirtyDescriptorsOnly)
{
	size_t offset = m_currentOffset;

	bool copyAllDescriptors = !copyDirtyDescriptorsOnly;

	for (uint8_t i = 0; i < descriptorCache.numActiveTables; ++i)
	{
		const uint8_t rootIndex = descriptorCache.activeTables[i];

		if (copyAllDescriptors || (copyDirtyDescriptorsOnly && descriptorCache.hasDirtyData[rootIndex]))
		{
			size_t tableSize = descriptorCache.tableSizes[rootIndex];
			memcpy(m_bufferStart + offset, descriptorCache.tableAllocations[rootIndex].mem, tableSize);
			descriptorCache.bindingOffsets[rootIndex] = offset;
			offset += tableSize;
		}
	}

	m_currentOffset += offset;
	m_freeSpace -= offset;
}


void DynamicDescriptorBuffer::BindDescriptorSetOffsets(VkCommandBuffer commandBuffer, DynamicDescriptorBuffer::DescriptorCache& descriptorCache, bool bindDirtyDescriptorSetsOnly, bool graphicsPipe)
{ 
	bool bindAllDescriptorSets = !bindDirtyDescriptorSetsOnly;

	// Bind the buffer offsets
	VkPipelineLayout pipelineLayout = descriptorCache.pipelineLayout;
	uint32_t bufferIndex = m_bufferType == DescriptorHeapType::Resource ? 0 : 1;

	for (uint8_t i = 0; i < descriptorCache.numActiveTables; ++i)
	{
		const uint8_t rootIndex = descriptorCache.activeTables[i];

		if (bindAllDescriptorSets || (bindDirtyDescriptorSetsOnly && descriptorCache.hasDirtyData[rootIndex]))
		{
			VkDeviceSize offset = descriptorCache.bindingOffsets[rootIndex];

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

		descriptorCache.hasDirtyData[rootIndex] = false;
	}
}

} // namespace Luna::VK