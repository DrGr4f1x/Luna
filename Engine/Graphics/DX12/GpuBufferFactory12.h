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


namespace Luna
{

// Forward declarations
class IResourceManager;

} // namespace Luna


namespace Luna::DX12
{

struct GpuBufferData
{
	wil::com_ptr<D3D12MA::Allocation> allocation; // TODO: move this elsewhere, it's not the hot data
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle{};
	D3D12_CPU_DESCRIPTOR_HANDLE uavHandle{};
	D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle{};
};


class GpuBufferFactory : public GpuBufferFactoryBase
{
public:
	GpuBufferFactory(IResourceManager* owner, ID3D12Device* device, D3D12MA::Allocator* allocator);

	ResourceHandle CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc);
	void Destroy(uint32_t index);

	// General resource methods
	ResourceType GetResourceType(uint32_t index) const
	{
		return m_descs[index].resourceType;
	}


	ResourceState GetUsageState(uint32_t index) const
	{
		return m_resources[index].usageState;
	}


	void SetUsageState(uint32_t index, ResourceState newState)
	{
		m_resources[index].usageState = newState;
	}


	ID3D12Resource* GetResource(uint32_t index) const
	{
		return m_resources[index].resource.get();
	}


	D3D12_CPU_DESCRIPTOR_HANDLE GetSRV(uint32_t index) const
	{
		return m_data[index].srvHandle;
	}


	D3D12_CPU_DESCRIPTOR_HANDLE GetUAV(uint32_t index) const
	{
		return m_data[index].uavHandle;
	}


	D3D12_CPU_DESCRIPTOR_HANDLE GetCBV(uint32_t index) const
	{
		return m_data[index].cbvHandle;
	}

	void Update(uint32_t index, size_t sizeInBytes, size_t offset, const void* data) const;

private:
	wil::com_ptr<D3D12MA::Allocation> AllocateBuffer(const GpuBufferDesc& gpuBufferDesc) const;

	void ResetData(uint32_t index);
	void ResetResource(uint32_t index);
	void ResetAllocation(uint32_t index);

	void ClearData();
	void ClearResources();
	void ClearAllocations();

private:
	IResourceManager* m_owner{ nullptr };
	wil::com_ptr<ID3D12Device> m_device;
	wil::com_ptr<D3D12MA::Allocator> m_allocator;

	std::mutex m_mutex;

	std::queue<uint32_t> m_freeList;

	std::array<ResourceData, MaxResources> m_resources;
	std::array<wil::com_ptr<D3D12MA::Allocation>, MaxResources> m_allocations;
	std::array<GpuBufferData, MaxResources> m_data;
};

} // namespace Luna::DX12