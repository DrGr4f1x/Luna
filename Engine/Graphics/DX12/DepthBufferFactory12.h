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

#include "Graphics\DepthBuffer.h"
#include "Graphics\DX12\DirectXCommon.h"


namespace Luna
{

// Forward declarations
class IResourceManager;

} // namespace Luna


namespace Luna::DX12
{

struct DepthBufferData
{
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 4> dsvHandles{};
	D3D12_CPU_DESCRIPTOR_HANDLE depthSrvHandle{};
	D3D12_CPU_DESCRIPTOR_HANDLE stencilSrvHandle{};
};


class DepthBufferFactory : public DepthBufferFactoryBase
{
public:
	DepthBufferFactory(IResourceManager* owner, ID3D12Device* device, D3D12MA::Allocator* allocator);

	ResourceHandle CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc);
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


	D3D12_CPU_DESCRIPTOR_HANDLE GetSRV(uint32_t index, bool depthSrv) const
	{
		return depthSrv ? m_data[index].depthSrvHandle : m_data[index].stencilSrvHandle;
	}


	D3D12_CPU_DESCRIPTOR_HANDLE GetDSV(uint32_t index, DepthStencilAspect depthStencilAspect) const
	{
		switch (depthStencilAspect)
		{
		case DepthStencilAspect::ReadWrite:		return m_data[index].dsvHandles[0];
		case DepthStencilAspect::ReadOnly:		return m_data[index].dsvHandles[1];
		case DepthStencilAspect::DepthReadOnly:	return m_data[index].dsvHandles[2];
		default:								return m_data[index].dsvHandles[3];
		}
	}

private:
	void ResetData(uint32_t index);
	void ResetResource(uint32_t index);

	void ClearData();
	void ClearResources();

private:
	IResourceManager* m_owner{ nullptr };
	wil::com_ptr<ID3D12Device> m_device;
	wil::com_ptr<D3D12MA::Allocator> m_allocator;

	std::mutex m_mutex;

	std::queue<uint32_t> m_freeList;

	std::array<ResourceData, MaxResources> m_resources;
	std::array<DepthBufferData, MaxResources> m_data;
};

} // namespace Luna::DX12