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

#include "DepthBufferPool12.h"

#include "Graphics\DX12\Device12.h"


namespace Luna::DX12
{

DepthBufferPool* g_depthBufferPool{ nullptr };


DepthBufferPool::DepthBufferPool(ID3D12Device* device, D3D12MA::Allocator* allocator)
	: m_device{ device }
	, m_allocator{ allocator }
{
	assert(g_depthBufferPool == nullptr);

	for (uint32_t i = 0; i < MaxItems; ++i)
	{
		m_freeList.push(i);
		m_descs[i] = DepthBufferDesc{};
		m_depthBufferData[i] = DepthBufferData{};
	}

	g_depthBufferPool = this;
}


DepthBufferPool::~DepthBufferPool()
{
	g_depthBufferPool = nullptr;
}


DepthBufferHandle DepthBufferPool::CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc)
{
	std::lock_guard guard(m_allocationMutex);

	assert(!m_freeList.empty());

	uint32_t index = m_freeList.front();
	m_freeList.pop();

	m_descs[index] = depthBufferDesc;
	m_depthBufferData[index] = CreateDepthBuffer_Internal(depthBufferDesc);

	return Create<DepthBufferHandleType>(index, this);
}


void DepthBufferPool::DestroyHandle(DepthBufferHandleType* handle)
{
	assert(handle != nullptr);

	std::lock_guard guard(m_allocationMutex);

	// TODO: queue this up to execute in one big batch, e.g. at the end of the frame

	uint32_t index = handle->GetIndex();
	m_descs[index] = DepthBufferDesc{};
	m_depthBufferData[index] = DepthBufferData{};

	m_freeList.push(index);
}


ResourceType DepthBufferPool::GetResourceType(DepthBufferHandleType* handle) const
{
	return GetDesc(handle).resourceType;
}


ResourceState DepthBufferPool::GetUsageState(DepthBufferHandleType* handle) const
{
	return GetData(handle).usageState;
}


void DepthBufferPool::SetUsageState(DepthBufferHandleType* handle, ResourceState newState)
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	m_depthBufferData[index].usageState = newState;
}


uint64_t DepthBufferPool::GetWidth(DepthBufferHandleType* handle) const
{
	return GetDesc(handle).width;
}


uint32_t DepthBufferPool::GetHeight(DepthBufferHandleType* handle) const
{
	return GetDesc(handle).height;
}


uint32_t DepthBufferPool::GetDepthOrArraySize(DepthBufferHandleType* handle) const
{
	return GetDesc(handle).arraySizeOrDepth;
}


uint32_t DepthBufferPool::GetNumMips(DepthBufferHandleType* handle) const
{
	return GetDesc(handle).numMips;
}


uint32_t DepthBufferPool::GetNumSamples(DepthBufferHandleType* handle) const
{
	return GetDesc(handle).numSamples;
}


uint32_t DepthBufferPool::GetPlaneCount(DepthBufferHandleType* handle) const
{
	return GetData(handle).planeCount;
}


Format DepthBufferPool::GetFormat(DepthBufferHandleType* handle) const
{
	return GetDesc(handle).format;
}


float DepthBufferPool::GetClearDepth(DepthBufferHandleType* handle) const
{
	return GetDesc(handle).clearDepth;
}


uint8_t DepthBufferPool::GetClearStencil(DepthBufferHandleType* handle) const
{
	return GetDesc(handle).clearStencil;
}


ID3D12Resource* DepthBufferPool::GetResource(DepthBufferHandleType* handle) const
{
	return GetData(handle).resource.get();
}


D3D12_CPU_DESCRIPTOR_HANDLE DepthBufferPool::GetSRV(DepthBufferHandleType* handle, bool depthSrv) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return depthSrv ? m_depthBufferData[index].depthSrvHandle : m_depthBufferData[index].stencilSrvHandle;
}


D3D12_CPU_DESCRIPTOR_HANDLE DepthBufferPool::GetDSV(DepthBufferHandleType* handle, DepthStencilAspect depthStencilAspect) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();

	switch (depthStencilAspect)
	{
	case DepthStencilAspect::ReadWrite:		return m_depthBufferData[index].dsvHandles[0];
	case DepthStencilAspect::ReadOnly:		return m_depthBufferData[index].dsvHandles[1];
	case DepthStencilAspect::DepthReadOnly:	return m_depthBufferData[index].dsvHandles[2];
	default:								return m_depthBufferData[index].dsvHandles[3];
	}
}


const DepthBufferDesc& DepthBufferPool::GetDesc(DepthBufferHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_descs[index];
}


const DepthBufferData& DepthBufferPool::GetData(DepthBufferHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_depthBufferData[index];
}


DepthBufferData DepthBufferPool::CreateDepthBuffer_Internal(const DepthBufferDesc& depthBufferDesc)
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
		.SampleDesc			= {.Count = depthBufferDesc.numSamples, .Quality = 0 },
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

	// TODO: this is just for allocating descriptor handles.  Think of another way to achieve this.
	auto device = GetD3D12GraphicsDevice();

	dsvHandles[0] = device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	dsvHandles[1] = device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	m_device->CreateDepthStencilView(resource.get(), &dsvDesc, dsvHandles[0]);

	dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
	m_device->CreateDepthStencilView(resource.get(), &dsvDesc, dsvHandles[1]);

	auto stencilReadFormat = GetStencilFormat(FormatToDxgi(depthBufferDesc.format).resourceFormat);
	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
	{
		dsvHandles[2] = device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		dsvHandles[3] = device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

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


	auto depthSrvHandle = device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
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
		stencilSrvHandle = device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		srvDesc.Format = stencilReadFormat;
		srvDesc.Texture2D.PlaneSlice = (srvDesc.Format == DXGI_FORMAT_X32_TYPELESS_G8X24_UINT) ? 1 : 0;
		m_device->CreateShaderResourceView(resource.get(), &srvDesc, stencilSrvHandle);
	}

	const uint8_t planeCount = device->GetFormatPlaneCount(FormatToDxgi(depthBufferDesc.format).resourceFormat);

	DepthBufferData data{
		.resource			= resource,
		.dsvHandles			= dsvHandles,
		.depthSrvHandle		= depthSrvHandle,
		.stencilSrvHandle	= stencilSrvHandle,
		.usageState			= ResourceState::Common,
		.planeCount			= planeCount
	};

	return data;
}


DepthBufferPool* const GetD3D12DepthBufferPool()
{
	return g_depthBufferPool;
}

} // namespace Luna::DX12