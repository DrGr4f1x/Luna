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
#include "Graphics\DX12\DirectXCommon.h"
#include "Graphics\DX12\DescriptorAllocator12.h"


namespace Luna::DX12
{

// Forward declarations
class Device;


class DescriptorSet : public IDescriptorSet
{
	friend class Device;

public:
	void SetSRV(uint32_t slot, ColorBufferPtr colorBuffer) override;
	void SetSRV(uint32_t slot, DepthBufferPtr depthBuffer, bool depthSrv = true) override;
	void SetSRV(uint32_t slot, GpuBufferPtr gpuBuffer) override;

	void SetUAV(uint32_t slot, ColorBufferPtr colorBuffer, uint32_t uavIndex = 0) override;
	void SetUAV(uint32_t slot, DepthBufferPtr depthBuffer) override;
	void SetUAV(uint32_t slot, GpuBufferPtr gpuBuffer) override;

	void SetCBV(uint32_t slot, GpuBufferPtr gpuBuffer) override;

	void SetDynamicOffset(uint32_t offset) override;

	void UpdateGpuDescriptors() override;

	bool HasBindableDescriptors() const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle() const;
	uint64_t GetGpuAddress() const;
	uint64_t GetDynamicOffset() const;
	uint64_t GetGpuAddressWithOffset() const;

protected:
	void SetDescriptor(uint32_t slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);

protected:
	Device* m_device{ nullptr };

	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, MaxDescriptorsPerTable> m_descriptors;
	DescriptorHandle m_descriptorHandle;
	uint64_t m_gpuAddress{ D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN };
	uint32_t m_numDescriptors{ 0 };
	uint32_t m_dirtyBits{ 0 };
	uint32_t m_dynamicOffset{ 0 };

	bool m_isSamplerTable{ false };
	bool m_isRootBuffer{ false };
};

} // namespace Luna::DX12