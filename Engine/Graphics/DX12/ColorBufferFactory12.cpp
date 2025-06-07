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

#include "ColorBufferFactory12.h"

#include "Graphics\ResourceManager.h"


namespace Luna::DX12
{

ColorBufferFactory::ColorBufferFactory(IResourceManager* owner, ID3D12Device* device, D3D12MA::Allocator* allocator)
	: ColorBufferFactoryBase()
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


ResourceHandle ColorBufferFactory::CreateColorBuffer(const ColorBufferDesc& colorBufferDesc)
{
	// Create resource
	auto numMips = colorBufferDesc.numMips == 0 ? ComputeNumMips(colorBufferDesc.width, colorBufferDesc.height) : colorBufferDesc.numMips;
	auto flags = CombineResourceFlags(colorBufferDesc.numSamples);

	D3D12_RESOURCE_DESC resourceDesc{
		.Dimension			= GetResourceDimension(colorBufferDesc.resourceType),
		.Alignment			= 0,
		.Width				= (UINT64)colorBufferDesc.width,
		.Height				= (UINT)colorBufferDesc.height,
		.DepthOrArraySize	= (UINT16)colorBufferDesc.arraySizeOrDepth,
		.MipLevels			= (UINT16)numMips,
		.Format				= FormatToDxgi(colorBufferDesc.format).resourceFormat,
		.SampleDesc			= { .Count = colorBufferDesc.numSamples, .Quality = 0 },
		.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN,
		.Flags				= (D3D12_RESOURCE_FLAGS)flags
	};

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = FormatToDxgi(colorBufferDesc.format).rtvFormat;
	clearValue.Color[0] = colorBufferDesc.clearColor.R();
	clearValue.Color[1] = colorBufferDesc.clearColor.G();
	clearValue.Color[2] = colorBufferDesc.clearColor.B();
	clearValue.Color[3] = colorBufferDesc.clearColor.A();

	ID3D12Resource* resource{ nullptr };
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	assert_succeeded(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&resourceDesc, D3D12_RESOURCE_STATE_COMMON, &clearValue, IID_PPV_ARGS(&resource)));

	SetDebugName(resource, colorBufferDesc.name);

	// Create descriptors and derived views
	assert_msg(colorBufferDesc.arraySizeOrDepth == 1 || numMips == 1, "We don't support auto-mips on texture arrays");

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};

	DXGI_FORMAT dxgiFormat = FormatToDxgi(colorBufferDesc.format).resourceFormat;
	rtvDesc.Format = dxgiFormat;
	uavDesc.Format = GetUAVFormat(dxgiFormat);
	srvDesc.Format = dxgiFormat;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	if (colorBufferDesc.arraySizeOrDepth > 1)
	{
		if (colorBufferDesc.resourceType == ResourceType::Texture3D)
		{
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
			rtvDesc.Texture3D.MipSlice = 0;
			rtvDesc.Texture3D.FirstWSlice = 0;
			rtvDesc.Texture3D.WSize = (UINT)colorBufferDesc.arraySizeOrDepth;

			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
			uavDesc.Texture3D.MipSlice = 0;
			uavDesc.Texture3D.FirstWSlice = 0;
			uavDesc.Texture3D.WSize = (UINT)colorBufferDesc.arraySizeOrDepth;

			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			srvDesc.Texture3D.MipLevels = numMips;
			srvDesc.Texture3D.MostDetailedMip = 0;
		}
		else
		{
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			rtvDesc.Texture2DArray.MipSlice = 0;
			rtvDesc.Texture2DArray.FirstArraySlice = 0;
			rtvDesc.Texture2DArray.ArraySize = (UINT)colorBufferDesc.arraySizeOrDepth;

			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			uavDesc.Texture2DArray.MipSlice = 0;
			uavDesc.Texture2DArray.FirstArraySlice = 0;
			uavDesc.Texture2DArray.ArraySize = (UINT)colorBufferDesc.arraySizeOrDepth;

			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Texture2DArray.MipLevels = numMips;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
			srvDesc.Texture2DArray.FirstArraySlice = 0;
			srvDesc.Texture2DArray.ArraySize = (UINT)colorBufferDesc.arraySizeOrDepth;
		}
	}
	else if (colorBufferDesc.numFragments > 1)
	{
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
	}
	else
	{
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;

		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;

		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = numMips;
		srvDesc.Texture2D.MostDetailedMip = 0;
	}

	auto rtvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	auto srvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Create the render target view
	m_device->CreateRenderTargetView(resource, &rtvDesc, rtvHandle);

	// Create the shader resource view
	m_device->CreateShaderResourceView(resource, &srvDesc, srvHandle);

	// Create the UAVs for each mip level (RWTexture2D)
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 12> uavHandles;
	if (colorBufferDesc.numFragments == 1)
	{
		for (uint32_t i = 0; i < (uint32_t)uavHandles.size(); ++i)
		{
			uavHandles[i].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		}

		for (uint32_t i = 0; i < numMips; ++i)
		{
			uavHandles[i] = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			m_device->CreateUnorderedAccessView(resource, nullptr, &uavDesc, uavHandles[i]);

			uavDesc.Texture2D.MipSlice++;
		}
	}

	const uint8_t planeCount = GetFormatPlaneCount(FormatToDxgi(colorBufferDesc.format).resourceFormat);

	ResourceData resourceData{
		.resource		= resource,
		.usageState		= ResourceState::Common
	};

	ColorBufferData colorBufferData{
		.srvHandle		= srvHandle,
		.rtvHandle		= rtvHandle,
		.uavHandles		= uavHandles
	};

	// Create handle and store cached data
	{
		std::lock_guard lock(m_mutex);

		assert(!m_freeList.empty());

		// Get an index allocation
		uint32_t index = m_freeList.front();
		m_freeList.pop();

		m_descs[index] = colorBufferDesc;
		m_descs[index].planeCount = planeCount;

		m_data[index] = colorBufferData;
		m_resources[index] = resourceData;

		return make_shared<ResourceHandleType>(index, IResourceManager::ManagedColorBuffer, m_owner);
	}
}


void ColorBufferFactory::Destroy(uint32_t index)
{
	std::lock_guard lock(m_mutex);

	ResetDesc(index);
	ResetData(index);
	ResetResource(index);

	m_freeList.push(index);
}


ResourceHandle ColorBufferFactory::CreateColorBufferFromSwapChain(IDXGISwapChain* swapChain, uint32_t imageIndex)
{
	wil::com_ptr<ID3D12Resource> displayPlane;
	assert_succeeded(swapChain->GetBuffer(imageIndex, IID_PPV_ARGS(&displayPlane)));

	const string name = format("Primary SwapChain Image {}", imageIndex);
	SetDebugName(displayPlane.get(), name);

	D3D12_RESOURCE_DESC resourceDesc = displayPlane->GetDesc();

	ColorBufferDesc colorBufferDesc{
		.name				= name,
		.resourceType		= ResourceType::Texture2D,
		.width				= resourceDesc.Width,
		.height				= resourceDesc.Height,
		.arraySizeOrDepth	= resourceDesc.DepthOrArraySize,
		.numSamples			= resourceDesc.SampleDesc.Count,
		.format				= DxgiToFormat(resourceDesc.Format)
	};

	auto rtvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_device->CreateRenderTargetView(displayPlane.get(), nullptr, rtvHandle);

	auto srvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_device->CreateShaderResourceView(displayPlane.get(), nullptr, srvHandle);

	const uint8_t planeCount = GetFormatPlaneCount(resourceDesc.Format);
	colorBufferDesc.planeCount = planeCount;

	ResourceData resourceData{
		.resource		= displayPlane.get(),
		.usageState		= ResourceState::Present
	};

	ColorBufferData colorBufferData{
		.srvHandle = srvHandle,
		.rtvHandle = rtvHandle
	};

	// Create handle and store cached data
	{
		std::lock_guard guard(m_mutex);

		assert(!m_freeList.empty());

		uint32_t index = m_freeList.front();
		m_freeList.pop();

		m_descs[index] = colorBufferDesc;
		m_data[index] = colorBufferData;
		m_resources[index] = resourceData;

		return make_shared<ResourceHandleType>(index, IResourceManager::ManagedColorBuffer, m_owner);
	}
}


void ColorBufferFactory::ResetData(uint32_t index)
{
	m_data[index] = ColorBufferData{};
}


void ColorBufferFactory::ResetResource(uint32_t index)
{
	m_resources[index].resource.reset();
	m_resources[index].usageState = ResourceState::Undefined;
}


void ColorBufferFactory::ClearData()
{
	for (uint32_t i = 0; i < MaxResources; ++i)
	{
		ResetData(i);
	}
}


void ColorBufferFactory::ClearResources()
{
	for (uint32_t i = 0; i < MaxResources; ++i)
	{
		ResetResource(i);
	}
}

} // namespace Luna::DX12