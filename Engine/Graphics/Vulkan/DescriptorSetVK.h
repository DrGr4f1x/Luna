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

	void SetSRV(uint32_t srvRegister, const IDescriptor* descriptor) override;
	void SetUAV(uint32_t uavRegister, const IDescriptor* descriptor) override;
	void SetCBV(uint32_t cbvRegister, const IDescriptor* descriptor) override;
	void SetSampler(uint32_t samplerRegister, const IDescriptor* descriptor) override;

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

	bool HasDescriptors() const;
	VkDescriptorSet GetDescriptorSet() const { return m_descriptorSet; }

	const DescriptorBindingTemplate* GetBindingTemplate() const { return m_template; }

protected:
	void UpdateDescriptorSet(const VkWriteDescriptorSet& writeDescriptorSet);
	template<bool isSrv>
	void SetSRVUAV(uint32_t srvUavRegister, const IDescriptor* descriptor);

	template <DescriptorRegisterType registerType>
	void SetDescriptors_Internal(uint32_t descriptorRegister, std::span<const IDescriptor*> descriptors);
	void WriteSamplers(VkWriteDescriptorSet& writeDescriptorSet, std::byte* scratchData, std::span<const IDescriptor*> descriptors);
	void WriteBuffers(VkWriteDescriptorSet& writeDescriptorSet, std::byte* scratchData, std::span<const IDescriptor*> descriptors);
	void WriteTextures(VkWriteDescriptorSet& writeDescriptorSet, std::byte* scratchData, std::span<const IDescriptor*> descriptors);
	void WriteTypedBuffers(VkWriteDescriptorSet& writeDescriptorSet, std::byte* scratchData, std::span<const IDescriptor*> descriptors);

protected:
	Device* m_device{ nullptr };

	RootParameter m_rootParameter;

	VkDescriptorSet m_descriptorSet{ VK_NULL_HANDLE };
	uint32_t m_numDescriptors{ 0 };

	DescriptorBindingTemplate* m_template{ nullptr };
};

} // namespace Luna::VK