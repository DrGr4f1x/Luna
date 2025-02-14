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

struct DescriptorSetDesc
{
	DescriptorHandle descriptorHandle;
	uint32_t numDescriptors{ 0 };
	bool isSamplerTable{ false };
	bool isRootBuffer{ false };
};


// TODO: Put descriptorHandle, gpuAddress, and dynamicOffset into a separate struct - they're the hot data for the GPU.
struct DescriptorSetData
{
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, MaxDescriptorsPerTable> descriptors;
	DescriptorHandle descriptorHandle;
	uint64_t gpuAddress{ D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN };
	uint32_t numDescriptors{ 0 };
	uint32_t dirtyBits{ 0 };
	uint32_t dynamicOffset{ 0 };

	bool isSamplerTable{ false };
	bool isRootBuffer{ false };
};


class DescriptorSetPool : public IDescriptorSetPool
{
	static const uint32_t MaxItems = (1 << 16);

public:
	explicit DescriptorSetPool(ID3D12Device* device);
	~DescriptorSetPool();

	// Create/Destroy descriptor set
	DescriptorSetHandle CreateDescriptorSet(const DescriptorSetDesc& descriptorSetDesc);
	void DestroyHandle(DescriptorSetHandleType* handle) override;

	// Platform agnostic functions
	void SetSRV(DescriptorSetHandleType* handle, int slot, const IColorBuffer* colorBuffer) override;
	void SetSRV(DescriptorSetHandleType* handle, int slot, const IDepthBuffer* depthBuffer, bool depthSrv = true) override;
	void SetSRV(DescriptorSetHandleType* handle, int slot, const IGpuBuffer* gpuBuffer) override;

	void SetUAV(DescriptorSetHandleType* handle, int slot, const IColorBuffer* colorBuffer, uint32_t uavIndex = 0) override;
	void SetUAV(DescriptorSetHandleType* handle, int slot, const IDepthBuffer* depthBuffer) override;
	void SetUAV(DescriptorSetHandleType* handle, int slot, const IGpuBuffer* gpuBuffer) override;

	void SetCBV(DescriptorSetHandleType* handle, int slot, const IGpuBuffer* gpuBuffer) override;

	void SetDynamicOffset(DescriptorSetHandleType* handle, uint32_t offset) override;

	void UpdateGpuDescriptors(DescriptorSetHandleType* handle) override;

	// Platform specific functions
	bool HasBindableDescriptors(DescriptorSetHandleType* handle) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(DescriptorSetHandleType* handle) const;
	uint64_t GetGpuAddress(DescriptorSetHandleType* handle) const;
	uint64_t GetDynamicOffset(DescriptorSetHandleType* handle) const;
	uint64_t GetGpuAddressWithOffset(DescriptorSetHandleType* handle) const;

private:
	void SetDescriptor(DescriptorSetData& data, int slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);

private:
	wil::com_ptr<ID3D12Device> m_device;

	// Allocation mutex
	std::mutex m_allocationMutex;

	// Free list
	std::queue<uint32_t> m_freeList;

	// Hot data
	std::array<DescriptorSetData, MaxItems> m_descriptorData;
};


DescriptorSetPool* const GetD3D12DescriptorSetPool();

} // namespace Luna::DX12