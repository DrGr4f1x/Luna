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

#include "Graphics\Vulkan\VulkanCommon.h"
#include "Graphics\RootSignature.h"
#include "Graphics\Vulkan\DescriptorAllocatorVK.h"


namespace Luna
{

class IColorBuffer;
class IDepthBuffer;
class IGpuBuffer;
class ISampler;
class ITexture;
struct RootParameter;

} // namespace Luna


namespace Luna::VK
{

// Forward declarations
class CommandContextVK;
class Descriptor;
class DescriptorSetLayout;
class RootSignature;


class DynamicDescriptorBuffer
{
public:
	DynamicDescriptorBuffer(CommandContextVK& owningContext, DescriptorBufferType bufferType);
	~DynamicDescriptorBuffer();

	static void DestroyAll();

	void CleanupUsedBuffers(uint64_t fenceValue);

	void ParseRootSignature(const RootSignature& rootSignature, bool graphicsPipe);

	void UpdateAndBindDescriptorSets(VkCommandBuffer commandBuffer, bool graphicsPipe);

	void SetSRV(CommandListType type, uint32_t rootIndex, uint32_t srvRegister, uint32_t arrayIndex, const IColorBuffer* colorBuffer);
	void SetSRV(CommandListType type, uint32_t rootIndex, uint32_t srvRegister, uint32_t arrayIndex, const IDepthBuffer* depthBuffer, bool depthSrv);
	void SetSRV(CommandListType type, uint32_t rootIndex, uint32_t srvRegister, uint32_t arrayIndex, const IGpuBuffer* gpuBuffer);
	void SetSRV(CommandListType type, uint32_t rootIndex, uint32_t srvRegister, uint32_t arrayIndex, const ITexture* texture);

	void SetUAV(CommandListType type, uint32_t rootIndex, uint32_t uavRegister, uint32_t arrayIndex, const IColorBuffer* colorBuffer);
	void SetUAV(CommandListType type, uint32_t rootIndex, uint32_t uavRegister, uint32_t arrayIndex, const IDepthBuffer* depthBuffer);
	void SetUAV(CommandListType type, uint32_t rootIndex, uint32_t uavRegister, uint32_t arrayIndex, const IGpuBuffer* gpuBuffer);

	void SetCBV(CommandListType type, uint32_t rootIndex, uint32_t cbvRegister, uint32_t arrayIndex, const IGpuBuffer* gpuBuffer);

	void SetSampler(CommandListType type, uint32_t rootIndex, uint32_t samplerRegister, uint32_t arrayIndex, const ISampler* sampler);

private:
	// Static methods
	static CVkBuffer* RequestDescriptorBuffer(DescriptorBufferType bufferType);
	static void DiscardDescriptorBuffers(DescriptorBufferType bufferType, uint64_t fenceValueForReset, const std::vector<CVkBuffer*>& usedBuffers);

	bool HasSpace(size_t neededSpace)
	{
		return (m_currentBufferPtr != nullptr && m_currentOffset + neededSpace <= m_freeSpace);
	}

	void RetireCurrentBuffer();
	void RetireUsedBuffers(uint64_t fenceValue);

	DescriptorBufferAllocation Allocate(size_t sizeInBytes);
	static size_t GetBufferSize(DescriptorBufferType bufferType);

private:
	// Static members
	static const size_t sm_numResourceDescriptors = (1 << 16);
	static const size_t sm_numSamplerDescriptors = (1 << 10);
	static const size_t sm_numCachedDescriptors = 32 * MaxRootParameters;
	static std::mutex sm_mutex;
	static std::vector<wil::com_ptr<CVkBuffer>> sm_descriptorBufferPool[2];
	static std::queue<std::pair<uint64_t, CVkBuffer*>> sm_retiredDescriptorBuffers[2];
	static std::queue<CVkBuffer*> sm_availableDescriptorBuffers[2];

	// Non-static members
	CommandContextVK& m_owningContext;
	CVkBuffer* m_currentBufferPtr{ nullptr };
	const DescriptorBufferType m_bufferType;

	std::byte* m_bufferStart{ nullptr };
	size_t m_currentOffset{ 0 };
	size_t m_freeSpace{ 0 };
	size_t m_offsetAlignment{ 0 };
	size_t m_rootSignatureSize{ 0 };

	std::vector<CVkBuffer*> m_retiredBuffers;

	struct DescriptorCache
	{
		// Pointers to mapped memory for writing descriptors with vkGetDescriptorEXT
		std::array<DescriptorBufferAllocation, MaxRootParameters> tableAllocations;

		// Data dirty flags
		std::array<bool, MaxRootParameters> hasActiveData;
		std::array<bool, MaxRootParameters> hasDirtyData;

		// Table sizes
		std::array<size_t, MaxRootParameters> tableSizes;

		// Table offsets in the target descriptor buffer 
		std::array<size_t, MaxRootParameters> bindingOffsets;

		// Remap table
		std::array<uint8_t, MaxRootParameters> activeTables;
		uint8_t numActiveTables{ 0 };

		// Descriptor set layouts
		std::array<DescriptorSetLayout*, MaxRootParameters> descriptorSetLayouts;

		// Cached descriptor memory
		std::byte* descriptorMemory{ nullptr };
		size_t allocatedSize{ 0 };	// Total size in bytes of the descriptorMemory buffer
		size_t currentOffset{ 0 };	// Current offset into descriptorMemory buffer
		size_t freeSpace{ 0 };		// Remaining free space (allocatedSize - currentOffset)

		// Total required size
		size_t totalSize{ 0 };		// Sum of tableSize, i.e. the total required bytes for all descriptors
									// for the current root signature (pipelineLayout)

		VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };

		void SetDescriptor(uint32_t rootIndex, uint32_t descriptorRegister, uint32_t arrayIndex, const Descriptor* descriptor);

		void Allocate(uint32_t rootIndex, size_t requiredSize, size_t offsetAlignment);

		void Clear();

		// Gets the size required by all the dirty tables
		size_t GetDirtySize() const;
	};

	DescriptorCache m_graphicsCache;
	DescriptorCache m_computeCache;

private:
	void CopyDescriptors(DescriptorCache& descriptorCache, bool copyDirtyDescriptorsOnly);
	void BindDescriptorSetOffsets(VkCommandBuffer commandBuffer, DescriptorCache& descriptorCache, bool bindDirtyDescriptorSetsOnly, bool graphicsPipe);
};

} // namespace Luna::VK