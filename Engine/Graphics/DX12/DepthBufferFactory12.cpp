//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#include "Stdafx.h"

#include "DepthBufferFactory12.h"

#include "Graphics\ResourceManager.h"


namespace Luna::DX12
{

DepthBufferFactory::DepthBufferFactory(IResourceManager* owner, ID3D12Device* device, D3D12MA::Allocator* allocator)
	: DepthBufferFactoryBase()
	, m_owner{ owner }
	, m_device{ device }
	, m_allocator{ allocator }
{
	for (uint32_t i = 0; i < MaxResources; ++i)
	{
		m_freeList.push(i);
	}

	ClearDescs();
	ClearData();
	ClearResources();
}


ResourceHandle DepthBufferFactory::CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc)
{ 
	// Create resource
	D3D12_RESOURCE_DESC resourceDesc{
		.Dimension			= GetResourceDimension(depthBufferDesc.resourceType),
		.Alignment			= 0,
		.Width				= (UINT64)depthBufferDesc.width,
		.Height				= (UINT)depthBufferDesc.height,
		.DepthOrArraySize	= (UINT16)depthBufferDesc.arraySizeOrDepth,
		.MipLevels			= (UINT16)depthBufferDesc.numMips,
		.Format				= FormatToDxgi(depthBufferDesc.format).resourceFormat,
		.SampleDesc			= { .Count = depthBufferDesc.numSamples, .Quality = 0 },
		.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN,
		.Flags				= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
	};

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = FormatToDxgi(depthBufferDesc.format).rtvFormat;
	clearValue.DepthStencil.Depth = depthBufferDesc.clearDepth;
	clearValue.DepthStencil.Stencil = depthBufferDesc.clearStencil;

	wil::com_ptr<ID3D12Resource> resource;
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	assert_succeeded(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&resourceDesc, D3D12_RESOURCE_STATE_COMMON, &clearValue, IID_PPV_ARGS(&resource)));

	SetDebugName(resource.get(), depthBufferDesc.name);

	// Create descriptors and derived views
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = GetDSVFormat(FormatToDxgi(depthBufferDesc.format).resourceFormat);

	if (resource->GetDesc().SampleDesc.Count == 1)
	{
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;
	}
	else
	{
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
	}

	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 4> dsvHandles{};
	for (auto& handle : dsvHandles)
	{
		handle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	dsvHandles[0] = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	dsvHandles[1] = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	m_device->CreateDepthStencilView(resource.get(), &dsvDesc, dsvHandles[0]);

	dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
	m_device->CreateDepthStencilView(resource.get(), &dsvDesc, dsvHandles[1]);

	auto stencilReadFormat = GetStencilFormat(FormatToDxgi(depthBufferDesc.format).resourceFormat);
	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
	{
		dsvHandles[2] = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		dsvHandles[3] = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		m_device->CreateDepthStencilView(resource.get(), &dsvDesc, dsvHandles[2]);

		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		m_device->CreateDepthStencilView(resource.get(), &dsvDesc, dsvHandles[3]);
	}
	else
	{
		dsvHandles[2] = dsvHandles[0];
		dsvHandles[3] = dsvHandles[1];
	}

	auto depthSrvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CPU_DESCRIPTOR_HANDLE stencilSrvHandle{};
	stencilSrvHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;

	// Create the shader resource view
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = GetDepthFormat(FormatToDxgi(depthBufferDesc.format).resourceFormat);
	if (dsvDesc.ViewDimension == D3D12_DSV_DIMENSION_TEXTURE2D)
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
	}
	else
	{
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
	}
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	m_device->CreateShaderResourceView(resource.get(), &srvDesc, depthSrvHandle);

	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
	{
		stencilSrvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		srvDesc.Format = stencilReadFormat;
		srvDesc.Texture2D.PlaneSlice = (srvDesc.Format == DXGI_FORMAT_X32_TYPELESS_G8X24_UINT) ? 1 : 0;
		m_device->CreateShaderResourceView(resource.get(), &srvDesc, stencilSrvHandle);
	}

	ResourceData resourceData{
		.resource		= resource,
		.usageState		= ResourceState::Common
	};

	DepthBufferData depthBufferData{
		.dsvHandles			= dsvHandles,
		.depthSrvHandle		= depthSrvHandle,
		.stencilSrvHandle	= stencilSrvHandle
	};

	// Create handle and store cached data
	{
		std::lock_guard lock(m_mutex);

		assert(!m_freeList.empty());

		// Get an index allocation
		uint32_t index = m_freeList.front();
		m_freeList.pop();

		m_descs[index] = depthBufferDesc;

		m_data[index] = depthBufferData;
		m_resources[index] = resourceData;

		return make_shared<ResourceHandleType>(index, IResourceManager::ManagedDepthBuffer, m_owner);
	}
}


void DepthBufferFactory::Destroy(uint32_t index)
{
	std::lock_guard lock(m_mutex);

	ResetDesc(index);
	ResetData(index);
	ResetResource(index);

	m_freeList.push(index);
}


void DepthBufferFactory::ResetData(uint32_t index)
{
	m_data[index] = DepthBufferData{};
}


void DepthBufferFactory::ResetResource(uint32_t index)
{
	m_resources[index].resource.reset();
	m_resources[index].usageState = ResourceState::Undefined;
}


void DepthBufferFactory::ClearData()
{
	for (uint32_t i = 0; i < MaxResources; ++i)
	{
		ResetData(i);
	}
}


void DepthBufferFactory::ClearResources()
{
	for (uint32_t i = 0; i < MaxResources; ++i)
	{
		ResetResource(i);
	}
}

} // namespace Luna::DX12