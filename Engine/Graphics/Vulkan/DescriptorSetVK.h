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

#include "Graphics\DescriptorSet.h"
#include "Graphics\RootSignature.h"
#include "Graphics\Vulkan\VulkanCommon.h"
#if USE_DESCRIPTOR_BUFFERS
#include "Graphics\Vulkan\DescriptorAllocatorVK.h"
#include "Graphics\Vulkan\DescriptorSetLayoutVK.h"
#endif // USE_DESCRIPTOR_BUFFERS

namespace Luna::VK
{

// Forward declarations
class DescriptorBindingTemplate;
class Device;


class DescriptorSet : public IDescriptorSet
{
	friend class Device;

public:
	DescriptorSet(Device* device, const RootParameter& rootParameter);

	void SetBindlessSRVs(uint32_t srvRegister, std::span<const IDescriptor*> descriptors) override;

	void SetSRV(uint32_t srvRegister, ColorBufferPtr colorBuffer) override;
	void SetSRV(uint32_t srvRegister, DepthBufferPtr depthBuffer, bool depthSrv = true) override;
	void SetSRV(uint32_t srvRegister, GpuBufferPtr gpuBuffer) override;
	void SetSRV(uint32_t srvRegister, TexturePtr texture) override;

	void SetUAV(uint32_t uavRegister, ColorBufferPtr colorBuffer, uint32_t uavIndex = 0) override;
	void SetUAV(uint32_t uavRegister, DepthBufferPtr depthBuffer) override;
	void SetUAV(uint32_t uavRegister, GpuBufferPtr gpuBuffer) override;

	void SetCBV(uint32_t cbvRegister, GpuBufferPtr gpuBuffer) override;

	void SetSampler(uint32_t samplerRegister, SamplerPtr sampler) override;

#if USE_DESCRIPTOR_BUFFERS
	size_t GetDescriptorBufferOffset() const;
#endif // USE_DESCRIPTOR_BUFFERS

#if USE_LEGACY_DESCRIPTOR_SETS
	bool HasDescriptors() const;
	VkDescriptorSet GetDescriptorSet() const { return m_descriptorSet; }
#endif // USE_LEGACY_DESCRIPTOR_SETS

protected:
	template <DescriptorRegisterType registerType>
	void SetDescriptors_Internal(uint32_t descriptorRegister, std::span<const IDescriptor*> descriptors);
	
#if USE_DESCRIPTOR_BUFFERS
	void SetTextureSRV_Internal(uint32_t srvRegister, uint32_t arrayIndex, VkImageView imageView);
	void SetTextureUAV_Internal(uint32_t uavRegister, uint32_t arrayIndex, VkImageView imageView);

	void SetBufferSRV_Internal(uint32_t srvRegister, uint32_t arrayIndex, VkBuffer buffer, size_t bufferSize);
	void SetBufferUAV_Internal(uint32_t uavRegister, uint32_t arrayIndex, VkBuffer buffer, size_t bufferSize);

	void SetTypedBufferSRV_Internal(uint32_t srvRegister, uint32_t arrayIndex, VkBuffer buffer, VkFormat format, size_t bufferSize);
	void SetTypedBufferUAV_Internal(uint32_t uavRegister, uint32_t arrayIndex, VkBuffer buffer, VkFormat format, size_t bufferSize);

	void SetCBV_Internal(uint32_t cbvRegister, uint32_t arrayIndex, VkBuffer buffer, size_t bufferSize);

	void SetSampler_Internal(uint32_t samplerRegister, uint32_t arrayIndex, VkSampler sampler);
#endif // USE_DESCRIPTOR_BUFFERS

#if USE_LEGACY_DESCRIPTOR_SETS
	void UpdateDescriptorSet(const VkWriteDescriptorSet& writeDescriptorSet);
	template<bool isSrv>
	void SetSRVUAV(uint32_t srvUavRegister, const IDescriptor* descriptor);

	void WriteSamplers(VkWriteDescriptorSet& writeDescriptorSet, std::byte* scratchData, std::span<const IDescriptor*> descriptors);
	void WriteBuffers(VkWriteDescriptorSet& writeDescriptorSet, std::byte* scratchData, std::span<const IDescriptor*> descriptors);
	void WriteTextures(VkWriteDescriptorSet& writeDescriptorSet, std::byte* scratchData, std::span<const IDescriptor*> descriptors);
	void WriteTypedBuffers(VkWriteDescriptorSet& writeDescriptorSet, std::byte* scratchData, std::span<const IDescriptor*> descriptors);
#endif // USE_LEGACY_DESCRIPTOR_SETS

protected:
	Device* m_device{ nullptr };

	RootParameter m_rootParameter;

#if USE_DESCRIPTOR_BUFFERS
	DescriptorBufferAllocation m_allocation{};
	DescriptorSetLayoutPtr m_layout;
#endif // USE_DESCRIPTOR_BUFFERS

#if USE_LEGACY_DESCRIPTOR_SETS
	VkDescriptorSet m_descriptorSet{ VK_NULL_HANDLE };
	uint32_t m_numDescriptors{ 0 };
#endif // USE_LEGACY_DESCRIPTOR_SETS
};

} // namespace Luna::VK