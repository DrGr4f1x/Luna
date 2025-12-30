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
#include "Graphics\DX12\DirectXCommon.h"
#include "Graphics\DX12\DescriptorAllocator12.h"


namespace Luna::DX12
{

// Forward declarations
class Descriptor;
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

	bool HasBindableDescriptors() const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle() const;
	uint64_t GetGpuAddress() const;
	uint64_t GetGpuAddressWithOffset() const;

protected:
	void UpdateDescriptor(uint32_t descriptorSlot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);

	uint32_t GetSrvOffset(uint32_t srvRegister) const;
	uint32_t GetCbvOffset(uint32_t cbvRegister) const;
	uint32_t GetUavOffset(uint32_t uavRegister) const;
	uint32_t GetSamplerOffset(uint32_t samplerRegister) const { return GetSrvOffset(samplerRegister); }

protected:
	Device* m_device{ nullptr };

	RootParameter m_rootParameter;

	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, MaxDescriptorsPerTable> m_descriptors;
	DescriptorHandle m_descriptorHandle;
	uint64_t m_gpuAddress{ D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN };
	uint32_t m_numDescriptors{ 0 };

	bool m_isSamplerTable{ false };

	// Offset tables (samplers use the SRV table)
	std::unordered_map<uint32_t, uint32_t> m_srvOffsets;
	std::unordered_map<uint32_t, uint32_t> m_cbvOffsets;
	std::unordered_map<uint32_t, uint32_t> m_uavOffsets;
};

} // namespace Luna::DX12