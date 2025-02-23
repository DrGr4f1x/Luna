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

#include "Graphics\GpuBuffer.h"
#include "Graphics\DX12\DirectXCommon.h"


namespace Luna::DX12
{

struct GpuBufferData
{
	wil::com_ptr<ID3D12Resource> resource;
	wil::com_ptr<D3D12MA::Allocation> allocation; // TODO: move this elsewhere, it's not the hot data
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle{};
	D3D12_CPU_DESCRIPTOR_HANDLE uavHandle{};
	D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle{};
	// TODO: Use D3D12 resource state directly
	ResourceState usageState{ ResourceState::Undefined };

	GpuBufferData& SetResource(ID3D12Resource* value) { resource = value; return *this; }
	GpuBufferData& SetAllocation(D3D12MA::Allocation* value) { allocation = value; return *this; }
	constexpr GpuBufferData& SetSrvHandle(D3D12_CPU_DESCRIPTOR_HANDLE value) noexcept { srvHandle = value; return *this; }
	constexpr GpuBufferData& SetUavHandle(D3D12_CPU_DESCRIPTOR_HANDLE value) noexcept { uavHandle = value; return *this; }
	constexpr GpuBufferData& SetCbvHandle(D3D12_CPU_DESCRIPTOR_HANDLE value) noexcept { cbvHandle = value; return *this; }
	constexpr GpuBufferData& SetUsageState(ResourceState value) noexcept { usageState = value; return *this; }
};


class GpuBufferManager : public IGpuBufferManager
{
	static const uint32_t MaxItems = (1 << 12);

public:
	GpuBufferManager(ID3D12Device* device, D3D12MA::Allocator* allocator);
	~GpuBufferManager();

	// Create/Destroy GpuBuffer
	GpuBufferHandle CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc) override;
	void DestroyHandle(GpuBufferHandleType* handle) override;

	// Platform agnostic functions
	ResourceType GetResourceType(GpuBufferHandleType* handle) const override;
	ResourceState GetUsageState(GpuBufferHandleType* handle) const override;
	void SetUsageState(GpuBufferHandleType* handle, ResourceState newState) override;
	size_t GetSize(GpuBufferHandleType* handle) const override;
	size_t GetElementCount(GpuBufferHandleType* handle) const override;
	size_t GetElementSize(GpuBufferHandleType* handle) const override;
	void Update(GpuBufferHandleType* handle, size_t sizeInBytes, size_t offset, const void* data) const override;

	// Platform specific functions
	ID3D12Resource* GetResource(GpuBufferHandleType* handle) const;
	uint64_t GetGpuAddress(GpuBufferHandleType* handle) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRV(GpuBufferHandleType* handle) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetUAV(GpuBufferHandleType* handle) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCBV(GpuBufferHandleType* handle) const;

private:
	GpuBufferData CreateBuffer_Internal(const GpuBufferDesc& gpuBufferDesc) const;
	wil::com_ptr<D3D12MA::Allocation> AllocateBuffer(const GpuBufferDesc& gpuBufferDesc) const;

private:
	wil::com_ptr<ID3D12Device> m_device;
	wil::com_ptr<D3D12MA::Allocator> m_allocator;

	// Allocation mutex
	std::mutex m_allocationMutex;

	// Free list
	std::queue<uint32_t> m_freeList;

	// Cold data
	std::array<GpuBufferDesc, MaxItems> m_descs;

	// Hot data
	std::array<GpuBufferData, MaxItems> m_gpuBufferData;
};


GpuBufferManager* const GetD3D12GpuBufferManager();

} // namespace Luna::DX12