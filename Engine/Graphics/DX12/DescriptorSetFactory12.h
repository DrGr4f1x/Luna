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

#include "Graphics\DX12\DirectXCommon.h"
#include "Graphics\DX12\DescriptorAllocator12.h"


namespace Luna
{

// Forward declarations
class ColorBuffer;
class DepthBuffer;
class GpuBuffer;
class IResourceManager;

} // namespace Luna


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


class DescriptorSetFactory
{
	static const uint32_t MaxResources = (1 << 10);
	static const uint32_t InvalidAllocation = ~0u;

public:
	DescriptorSetFactory(IResourceManager* owner, ID3D12Device* device);

	ResourceHandle CreateDescriptorSet(const DescriptorSetDesc& descriptorSetDesc);
	void Destroy(uint32_t index);

	void SetSRV(uint32_t index, int slot, const ColorBuffer& colorBuffer);
	void SetSRV(uint32_t index, int slot, const DepthBuffer& depthBuffer, bool depthSrv = true);
	void SetSRV(uint32_t index, int slot, const GpuBuffer& gpuBuffer);
	void SetUAV(uint32_t index, int slot, const ColorBuffer& colorBuffer, uint32_t uavIndex = 0);
	void SetUAV(uint32_t index, int slot, const DepthBuffer& depthBuffer);
	void SetUAV(uint32_t index, int slot, const GpuBuffer& gpuBuffer);
	void SetCBV(uint32_t index, int slot, const GpuBuffer& gpuBuffer);
	void SetDynamicOffset(uint32_t index, uint32_t offset);
	void UpdateGpuDescriptors(uint32_t index);

	bool HasBindableDescriptors(uint32_t index) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(uint32_t index) const;
	uint64_t GetDescriptorSetGpuAddress(uint32_t index) const;
	uint64_t GetDynamicOffset(uint32_t index) const;
	uint64_t GetGpuAddressWithOffset(uint32_t index) const;

private:
	void SetDescriptor(DescriptorSetData& data, int slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);

	void ResetDesc(uint32_t index)
	{
		m_descs[index] = DescriptorSetDesc{};
	}

	void ResetData(uint32_t index)
	{
		m_data[index] = DescriptorSetData{};
	}

	void ClearDescs()
	{
		for (uint32_t i = 0; i < MaxResources; ++i)
		{
			ResetDesc(i);
		}
	}

	void ClearData()
	{
		for (uint32_t i = 0; i < MaxResources; ++i)
		{
			ResetData(i);
		}
	}

private:
	IResourceManager* m_owner{ nullptr };
	wil::com_ptr<ID3D12Device> m_device;

	std::mutex m_mutex;

	std::queue<uint32_t> m_freeList;

	std::array<DescriptorSetDesc, MaxResources> m_descs;
	std::array<DescriptorSetData, MaxResources> m_data;
};

} // namespace Luna::DX12