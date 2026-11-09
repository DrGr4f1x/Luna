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
#include "Graphics\Vulkan\DescriptorAllocationVK.h"
#include "Graphics\Vulkan\DescriptorSetLayoutVK.h"

namespace Luna::VK
{

// Forward declarations
class DescriptorBindingTemplate;
class Descriptor;
class Device;


struct PushData
{
	uint32_t offset{ ~0u };
	uint32_t value{ ~0u };
};


class IDescriptorSetVK : public IDescriptorSet
{
public:
	virtual DescriptorManagement GetDescriptorManagement() const = 0;
};


class BufferDescriptorSet : public IDescriptorSetVK
{
	friend class Device;

public:
	BufferDescriptorSet(Device* device, const RootParameter& rootParameter);

	DescriptorManagement GetDescriptorManagement() const final { return DescriptorManagement::Buffer; }

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

	size_t GetDescriptorBufferOffset() const;

protected:
	template <DescriptorRegisterType registerType>
	void SetDescriptors_Internal(uint32_t descriptorRegister, std::span<const IDescriptor*> descriptors);

	void SetSRV_Internal(uint32_t srvRegister, const Descriptor* descriptor);
	void SetUAV_Internal(uint32_t uavRegister, const Descriptor* descriptor);
	void SetCBV_Internal(uint32_t cbvRegister, const Descriptor* descriptor);
	void SetSampler_Internal(uint32_t samplerRegister, const Descriptor* descriptor);

	size_t GetRegisterOffsetSRV(uint32_t srvRegister, uint32_t arrayIndex = 0, uint32_t descriptorSize = 0) const;
	size_t GetRegisterOffsetUAV(uint32_t uavRegister, uint32_t arrayIndex = 0, uint32_t descriptorSize = 0) const;
	size_t GetRegisterOffsetCBV(uint32_t cbvRegister, uint32_t arrayIndex = 0, uint32_t descriptorSize = 0) const;
	size_t GetRegisterOffsetSampler(uint32_t samplerRegister, uint32_t arrayIndex = 0, uint32_t descriptorSize = 0) const;

private:
	Device* m_device{ nullptr };

	RootParameter m_rootParameter;

	// Descriptor buffer
	DescriptorAllocation m_allocation{};
	DescriptorSetLayoutPtr m_layout;
};


class HeapDescriptorSet : public IDescriptorSetVK
{
	friend class Device;

public:
	HeapDescriptorSet(Device* device, const RootParameter& rootParameter);

	DescriptorManagement GetDescriptorManagement() const final { return DescriptorManagement::Heap; }

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

	const std::vector<PushData>& GetPushData() const { return m_pushData; }

protected:
	template <DescriptorRegisterType registerType>
	void SetDescriptors_Internal(uint32_t descriptorRegister, std::span<const IDescriptor*> descriptors);

	void SetSRV_Internal(uint32_t srvRegister, const Descriptor* descriptor);
	void SetUAV_Internal(uint32_t uavRegister, const Descriptor* descriptor);
	void SetCBV_Internal(uint32_t cbvRegister, const Descriptor* descriptor);
	void SetSampler_Internal(uint32_t samplerRegister, const Descriptor* descriptor);

private:
	Device* m_device{ nullptr };

	RootParameter m_rootParameter;

	// Push data values
	std::vector<PushData> m_pushData;

	// Descriptor allocations per range
	std::vector<DescriptorAllocation> m_rangeAllocations;

	// Maps for registers to heap memory offsets
	HeapDescriptorBindingMapPtr m_registerOffsetMap;
};

} // namespace Luna::VK