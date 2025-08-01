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

#include "Device12.h"

#include "FileSystem.h"

#include "Graphics\CommandContext.h"

#include "ColorBuffer12.h"
#include "DepthBuffer12.h"
#include "DescriptorSet12.h"
#include "GpuBuffer12.h"
#include "PipelineState12.h"
#include "QueryHeap12.h"
#include "RootSignature12.h"
#include "Sampler12.h"
#include "Shader12.h"
#include "Texture12.h"

using namespace std;


namespace Luna::DX12
{

static Device* g_d3d12Device{ nullptr };


pair<string, bool> GetShaderFilenameWithExtension(const string& shaderFilename)
{
	auto fileSystem = GetFileSystem();

	string shaderFileWithExtension = shaderFilename;
	bool exists = false;

	// See if the filename already has an extension
	string extension = fileSystem->GetFileExtension(shaderFilename);
	if (!extension.empty())
	{
		exists = fileSystem->Exists(shaderFileWithExtension);
	}
	else
	{
		// Try .dxil extension first
		shaderFileWithExtension = shaderFilename + ".dxil";
		exists = fileSystem->Exists(shaderFileWithExtension);
		if (!exists)
		{
			// Try .dxbc next
			shaderFileWithExtension = shaderFilename + ".dxbc";
			exists = fileSystem->Exists(shaderFileWithExtension);
		}
	}

	return make_pair(shaderFileWithExtension, exists);
}


Shader* LoadShader(ShaderType type, const ShaderNameAndEntry& shaderNameAndEntry)
{
	auto [shaderFilenameWithExtension, exists] = GetShaderFilenameWithExtension(shaderNameAndEntry.shaderFile);

	if (!exists)
	{
		return nullptr;
	}

	ShaderDesc shaderDesc{
		.filenameWithExtension = shaderFilenameWithExtension,
		.entry = shaderNameAndEntry.entry,
		.type = type
	};

	return Shader::Load(shaderDesc);
}


Device::Device(ID3D12Device* device, D3D12MA::Allocator* allocator)
	: m_device{ device }
	, m_allocator{ allocator }
{
	assert(g_d3d12Device == nullptr);
	g_d3d12Device = this;
}


Luna::ColorBufferPtr Device::CreateColorBuffer(const ColorBufferDesc& colorBufferDesc)
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

	wil::com_ptr<ID3D12Resource> resource;
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	assert_succeeded(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&resourceDesc, D3D12_RESOURCE_STATE_COMMON, &clearValue, IID_PPV_ARGS(&resource)));

	SetDebugName(resource.get(), colorBufferDesc.name);

	// Create descriptors and derived views
	assert_msg(colorBufferDesc.arraySizeOrDepth == 1 || numMips == 1, "We don't support auto-mips on texture arrays");

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};

	DXGI_FORMAT dxgiFormat = FormatToDxgi(colorBufferDesc.format).resourceFormat;
	rtvDesc.Format = FormatToDxgi(colorBufferDesc.format).rtvFormat;
	srvDesc.Format = FormatToDxgi(colorBufferDesc.format).srvFormat;
	uavDesc.Format = srvDesc.Format;
	
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
			if (colorBufferDesc.numSamples > 1)
			{
				rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
			}
			else
			{
				rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			}
			
			rtvDesc.Texture2DArray.MipSlice = 0;
			rtvDesc.Texture2DArray.FirstArraySlice = 0;
			rtvDesc.Texture2DArray.ArraySize = (UINT)colorBufferDesc.arraySizeOrDepth;

			uavDesc.Texture2DArray.MipSlice = 0;
			uavDesc.Texture2DArray.FirstArraySlice = 0;
			uavDesc.Texture2DArray.ArraySize = (UINT)colorBufferDesc.arraySizeOrDepth;

			srvDesc.Texture2DArray.MipLevels = numMips;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
			srvDesc.Texture2DArray.FirstArraySlice = 0;
			srvDesc.Texture2DArray.ArraySize = (UINT)colorBufferDesc.arraySizeOrDepth;
		}
	}
	else
	{
		if (colorBufferDesc.numSamples > 1)
		{
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMS;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
		}
		else
		{
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		}

		rtvDesc.Texture2D.MipSlice = 0;

		uavDesc.Texture2D.MipSlice = 0;

		srvDesc.Texture2D.MipLevels = numMips;
		srvDesc.Texture2D.MostDetailedMip = 0;
	}

	auto rtvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	auto srvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Create the render target view
	m_device->CreateRenderTargetView(resource.get(), &rtvDesc, rtvHandle);

	// Create the shader resource view
	m_device->CreateShaderResourceView(resource.get(), &srvDesc, srvHandle);

	// Create the UAVs for each mip level (RWTexture2D)
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 12> uavHandles;
	for (uint32_t i = 0; i < (uint32_t)uavHandles.size(); ++i)
	{
		uavHandles[i].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	if (colorBufferDesc.numSamples == 1)
	{
		for (uint32_t i = 0; i < numMips; ++i)
		{
			uavHandles[i] = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			m_device->CreateUnorderedAccessView(resource.get(), nullptr, &uavDesc, uavHandles[i]);

			uavDesc.Texture2D.MipSlice++;
		}
	}

	const uint8_t planeCount = GetFormatPlaneCount(FormatToDxgi(colorBufferDesc.format).resourceFormat);

	auto colorBuffer = std::make_shared<Luna::DX12::ColorBuffer>();

	colorBuffer->m_device = this;
	colorBuffer->m_type = colorBufferDesc.resourceType;
	colorBuffer->m_width = colorBufferDesc.width;
	colorBuffer->m_height = colorBufferDesc.height;
	colorBuffer->m_arraySizeOrDepth = colorBufferDesc.arraySizeOrDepth;
	colorBuffer->m_numMips = numMips;
	colorBuffer->m_numSamples = colorBufferDesc.numSamples;
	colorBuffer->m_planeCount = planeCount;
	colorBuffer->m_format = colorBufferDesc.format;
	colorBuffer->m_dimension = ResourceTypeToTextureDimension(colorBuffer->m_type);
	colorBuffer->m_clearColor = colorBufferDesc.clearColor;
	colorBuffer->m_resource = resource;
	colorBuffer->m_srvHandle = srvHandle;
	colorBuffer->m_rtvHandle = rtvHandle;
	for (uint32_t i = 0; i < uavHandles.size(); ++i)
	{
		colorBuffer->m_uavHandles[i] = uavHandles[i];
	}

	return colorBuffer;
}


Luna::DepthBufferPtr Device::CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc)
{
	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	if (!depthBufferDesc.createShaderResources)
	{
		flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
	}

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
		.Flags				= flags
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

	D3D12_CPU_DESCRIPTOR_HANDLE depthSrvHandle{ .ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN };
	D3D12_CPU_DESCRIPTOR_HANDLE stencilSrvHandle{ .ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN };

	if (depthBufferDesc.createShaderResources)
	{
		depthSrvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

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
	}

	const uint8_t planeCount = GetFormatPlaneCount(FormatToDxgi(depthBufferDesc.format).resourceFormat);

	auto depthBuffer = std::make_shared<Luna::DX12::DepthBuffer>();

	depthBuffer->m_device = this;
	depthBuffer->m_type = depthBufferDesc.resourceType;
	depthBuffer->m_width = depthBufferDesc.width;
	depthBuffer->m_height = depthBufferDesc.height;
	depthBuffer->m_arraySizeOrDepth = depthBufferDesc.arraySizeOrDepth;
	depthBuffer->m_numMips = 1;
	depthBuffer->m_numSamples = depthBufferDesc.numSamples;
	depthBuffer->m_planeCount = planeCount;
	depthBuffer->m_format = depthBufferDesc.format;
	depthBuffer->m_dimension = ResourceTypeToTextureDimension(depthBuffer->m_type);
	depthBuffer->m_clearDepth = depthBufferDesc.clearDepth;
	depthBuffer->m_clearStencil = depthBufferDesc.clearStencil;
	depthBuffer->m_resource = resource;
	for (uint32_t i = 0; i < dsvHandles.size(); ++i)
	{
		depthBuffer->m_dsvHandles[i] = dsvHandles[i];
	}
	depthBuffer->m_depthSrvHandle = depthSrvHandle;
	depthBuffer->m_stencilSrvHandle = stencilSrvHandle;

	return depthBuffer;
}


Luna::GpuBufferPtr Device::CreateGpuBuffer(const GpuBufferDesc& gpuBufferDescIn)
{
	ResourceState initialState = ResourceState::GenericRead;

	GpuBufferDesc gpuBufferDesc = gpuBufferDescIn;
	if (gpuBufferDescIn.resourceType == ResourceType::ConstantBuffer)
	{
		gpuBufferDesc.elementSize = Math::AlignUp(gpuBufferDescIn.elementSize, 256);
	}

	wil::com_ptr<D3D12MA::Allocation> allocation = AllocateBuffer(gpuBufferDesc);
	ID3D12Resource* pResource = allocation->GetResource();

	SetDebugName(pResource, gpuBufferDesc.name);

	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle{};
	D3D12_CPU_DESCRIPTOR_HANDLE uavHandle{};
	D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle{};

	const size_t bufferSize = gpuBufferDesc.elementCount * gpuBufferDesc.elementSize;

	if (gpuBufferDesc.resourceType == ResourceType::ByteAddressBuffer || gpuBufferDesc.resourceType == ResourceType::IndirectArgsBuffer)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
			.Format						= DXGI_FORMAT_R32_TYPELESS,
			.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER,
			.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Buffer	= {
				.NumElements	= (uint32_t)bufferSize / 4,
				.Flags			= D3D12_BUFFER_SRV_FLAG_RAW
			}
		};

		srvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateShaderResourceView(pResource, &srvDesc, srvHandle);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{
			.Format				= DXGI_FORMAT_R32_TYPELESS,
			.ViewDimension		= D3D12_UAV_DIMENSION_BUFFER,
			.Buffer	= {
				.NumElements	= (uint32_t)bufferSize / 4,
				.Flags			= D3D12_BUFFER_UAV_FLAG_RAW
			}
		};

		uavHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateUnorderedAccessView(pResource, nullptr, &uavDesc, uavHandle);
	}

	if (gpuBufferDesc.resourceType == ResourceType::StructuredBuffer)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
			.Format						= DXGI_FORMAT_UNKNOWN,
			.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER,
			.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Buffer = {
				.NumElements			= (uint32_t)gpuBufferDesc.elementCount,
				.StructureByteStride	= (uint32_t)gpuBufferDesc.elementSize,
				.Flags					= D3D12_BUFFER_SRV_FLAG_NONE
			}
		};

		srvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateShaderResourceView(pResource, &srvDesc, srvHandle);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{
			.Format				= DXGI_FORMAT_UNKNOWN,
			.ViewDimension		= D3D12_UAV_DIMENSION_BUFFER,
			.Buffer = {
				.NumElements				= (uint32_t)gpuBufferDesc.elementCount,
				.StructureByteStride		= (uint32_t)gpuBufferDesc.elementSize,
				.CounterOffsetInBytes		= 0,
				.Flags						= D3D12_BUFFER_UAV_FLAG_NONE
			}
		};

		uavHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateUnorderedAccessView(pResource, nullptr, &uavDesc, uavHandle);
	}

	if (gpuBufferDesc.resourceType == ResourceType::TypedBuffer)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
			.Format						= FormatToDxgi(gpuBufferDesc.format).resourceFormat,
			.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER,
			.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Buffer = {
				.NumElements	= (uint32_t)gpuBufferDesc.elementCount,
				.Flags			= D3D12_BUFFER_SRV_FLAG_NONE
			}
		};

		srvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateShaderResourceView(pResource, &srvDesc, srvHandle);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{
			.Format				= FormatToDxgi(gpuBufferDesc.format).resourceFormat,
			.ViewDimension		= D3D12_UAV_DIMENSION_BUFFER,
			.Buffer = {
				.NumElements	= (uint32_t)gpuBufferDesc.elementCount,
				.Flags			= D3D12_BUFFER_UAV_FLAG_NONE
			}
		};

		uavHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateUnorderedAccessView(pResource, nullptr, &uavDesc, uavHandle);
	}

	if (gpuBufferDesc.resourceType == ResourceType::ConstantBuffer)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{
			.BufferLocation		= pResource->GetGPUVirtualAddress(),
			.SizeInBytes		= (uint32_t)(gpuBufferDesc.elementCount * gpuBufferDesc.elementSize)
		};

		cbvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_device->CreateConstantBufferView(&cbvDesc, cbvHandle);
	}

	if (gpuBufferDesc.resourceType == ResourceType::VertexBuffer || gpuBufferDesc.resourceType == ResourceType::IndexBuffer)
	{
		if (gpuBufferDesc.bAllowShaderResource)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
				.Format						= FormatToDxgi(gpuBufferDesc.format).resourceFormat,
				.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER,
				.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
				.Buffer = {
					.NumElements			= (uint32_t)gpuBufferDesc.elementCount,
					.StructureByteStride	= (uint32_t)gpuBufferDesc.elementSize,
					.Flags					= D3D12_BUFFER_SRV_FLAG_NONE
				}
			};

			srvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			m_device->CreateShaderResourceView(pResource, &srvDesc, srvHandle);
		}

		if (gpuBufferDesc.bAllowUnorderedAccess)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{
				.Format			= FormatToDxgi(gpuBufferDesc.format).resourceFormat,
				.ViewDimension	= D3D12_UAV_DIMENSION_BUFFER,
				.Buffer = {
					.NumElements			= (uint32_t)gpuBufferDesc.elementCount,
					.StructureByteStride	= (uint32_t)gpuBufferDesc.elementSize,
					.CounterOffsetInBytes	= 0,
					.Flags					= D3D12_BUFFER_UAV_FLAG_NONE
				}
			};

			uavHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			m_device->CreateUnorderedAccessView(pResource, nullptr, &uavDesc, uavHandle);
		}
	}

	auto gpuBuffer = std::make_shared<GpuBuffer>();

	gpuBuffer->m_device = this;
	gpuBuffer->m_type = gpuBufferDesc.resourceType;
	gpuBuffer->m_usageState = initialState;
	gpuBuffer->m_format = gpuBufferDesc.format;
	gpuBuffer->m_elementSize = gpuBufferDescIn.elementSize;
	gpuBuffer->m_elementCount = gpuBufferDesc.elementCount;
	gpuBuffer->m_bufferSize = gpuBuffer->m_elementCount * gpuBuffer->m_elementSize;
	gpuBuffer->m_allocation = allocation;
	gpuBuffer->m_srvHandle = srvHandle;
	gpuBuffer->m_uavHandle = uavHandle;
	gpuBuffer->m_cbvHandle = cbvHandle;
	gpuBuffer->m_isCpuWriteable = HasFlag(gpuBufferDesc.memoryAccess, MemoryAccess::CpuWrite);

	if (gpuBufferDescIn.initialData)
	{
		if (gpuBuffer->m_type == ResourceType::ConstantBuffer)
		{
			const size_t initialSize = gpuBuffer->GetBufferSize();
			gpuBuffer->Update(initialSize, gpuBufferDesc.initialData);
		}
		else
		{
			GpuBufferPtr temp = gpuBuffer;
			CommandContext::InitializeBuffer(temp, gpuBufferDescIn.initialData, gpuBuffer->GetBufferSize());
		}
	}

	return gpuBuffer;
}


Luna::RootSignaturePtr Device::CreateRootSignature(const RootSignatureDesc& rootSignatureDesc)
{
	std::vector<D3D12_ROOT_PARAMETER> d3d12RootParameters;

	auto exitGuard = wil::scope_exit([&]()
		{
			for (auto& param : d3d12RootParameters)
			{
				if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE && 
					param.DescriptorTable.NumDescriptorRanges > 0)
				{
					delete[] param.DescriptorTable.pDescriptorRanges;
				}
			}
		});

	// Validate RootSignatureDesc
	if (!rootSignatureDesc.Validate())
	{
		LogError(LogDirectX) << "RootSignature is not valid!" << endl;
		return nullptr;
	}

	// Build DX12 root parameter descriptions
	for (const auto& rootParameter : rootSignatureDesc.rootParameters)
	{
		if (rootParameter.parameterType == RootParameterType::RootConstants)
		{
			D3D12_ROOT_PARAMETER& param = d3d12RootParameters.emplace_back();
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
			param.ShaderVisibility = ShaderStageToDX12(rootParameter.shaderVisibility);
			param.Constants.Num32BitValues = rootParameter.num32BitConstants;
			param.Constants.RegisterSpace = rootParameter.registerSpace;
			param.Constants.ShaderRegister = rootParameter.startRegister;
		}
		else if (rootParameter.parameterType == RootParameterType::RootCBV ||
			rootParameter.parameterType == RootParameterType::RootSRV ||
			rootParameter.parameterType == RootParameterType::RootUAV)
		{
			D3D12_ROOT_PARAMETER& param = d3d12RootParameters.emplace_back();
			param.ParameterType = RootParameterTypeToDX12(rootParameter.parameterType);
			param.ShaderVisibility = ShaderStageToDX12(rootParameter.shaderVisibility);
			//param.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
			param.Descriptor.RegisterSpace = rootParameter.registerSpace;
			param.Descriptor.ShaderRegister = rootParameter.startRegister;
		}
		else if (rootParameter.parameterType == RootParameterType::Table)
		{
			D3D12_ROOT_PARAMETER& param = d3d12RootParameters.emplace_back();
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			param.ShaderVisibility = ShaderStageToDX12(rootParameter.shaderVisibility);

			uint32_t currentRegister[] = { 0, 0, 0, 0 };

			const uint32_t numRanges = (uint32_t)rootParameter.table.size();
			param.DescriptorTable.NumDescriptorRanges = numRanges;
			D3D12_DESCRIPTOR_RANGE* pRanges = new D3D12_DESCRIPTOR_RANGE[numRanges];
			for (uint32_t i = 0; i < numRanges; ++i)
			{
				D3D12_DESCRIPTOR_RANGE& d3d12Range = pRanges[i];
				const DescriptorRange& range = rootParameter.table[i];
				d3d12Range.RangeType = DescriptorTypeToDX12(range.descriptorType);
				d3d12Range.NumDescriptors = range.numDescriptors;

				if (range.startRegister == APPEND_REGISTER)
				{
					d3d12Range.BaseShaderRegister = currentRegister[d3d12Range.RangeType];
					currentRegister[d3d12Range.RangeType] += d3d12Range.NumDescriptors;
				}
				else
				{
					d3d12Range.BaseShaderRegister = range.startRegister;
					currentRegister[d3d12Range.RangeType] = d3d12Range.BaseShaderRegister + d3d12Range.NumDescriptors;
				}

				d3d12Range.RegisterSpace = range.registerSpace;
				d3d12Range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
				// TODO: Revisit this flag
				/*if (d3d12Range.RangeType != D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
				{
					d3d12Range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
				}
				else
				{
					d3d12Range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
				}*/
			}
			param.DescriptorTable.pDescriptorRanges = pRanges;
		}
	}

	// Build static samplers
	vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
	uint32_t currentSamplerRegister = 0;
	for (const auto& staticSamplerDesc : rootSignatureDesc.staticSamplers)
	{
		const auto& samplerDesc = staticSamplerDesc.samplerDesc;

		D3D12_STATIC_SAMPLER_DESC d3d12StaticSamplerDesc{
			.Filter				= TextureFilterToDX12(samplerDesc.filter),
			.AddressU			= TextureAddressToDX12(samplerDesc.addressU),
			.AddressV			= TextureAddressToDX12(samplerDesc.addressV),
			.AddressW			= TextureAddressToDX12(samplerDesc.addressW),
			.MipLODBias			= samplerDesc.mipLODBias,
			.MaxAnisotropy		= samplerDesc.maxAnisotropy,
			.ComparisonFunc		= ComparisonFuncToDX12(samplerDesc.comparisonFunc),
			.BorderColor		= BorderColorToDX12(samplerDesc.staticBorderColor),
			.MinLOD				= samplerDesc.minLOD,
			.MaxLOD				= samplerDesc.maxLOD,
			.RegisterSpace		= 0,
			.ShaderVisibility	= ShaderStageToDX12(staticSamplerDesc.shaderStage)
		};

		// Set shader register
		if (staticSamplerDesc.shaderRegister == APPEND_REGISTER)
		{
			d3d12StaticSamplerDesc.ShaderRegister = currentSamplerRegister;
			currentSamplerRegister += 1;
		}
		else
		{
			d3d12StaticSamplerDesc.ShaderRegister = staticSamplerDesc.shaderRegister;
			currentSamplerRegister = d3d12StaticSamplerDesc.ShaderRegister + 1;
		}

		staticSamplers.push_back(d3d12StaticSamplerDesc);
	}

	D3D12_ROOT_SIGNATURE_DESC d3d12RootSignatureDesc{
		.NumParameters		= (uint32_t)d3d12RootParameters.size(),
		.pParameters		= d3d12RootParameters.data(),
		.NumStaticSamplers	= (uint32_t)staticSamplers.size(),
		.pStaticSamplers	= staticSamplers.data(),
		.Flags				= RootSignatureFlagsToDX12(rootSignatureDesc.flags)
	};

	uint32_t descriptorTableBitmap{ 0 };
	uint32_t samplerTableBitmap{ 0 };
	std::vector<uint32_t> descriptorTableSize;
	descriptorTableSize.reserve(16);

	// Calculate hash
	size_t hashCode = 0;
	for (uint32_t param = 0; param < d3d12RootSignatureDesc.NumParameters; ++param)
	{
		const D3D12_ROOT_PARAMETER& rootParam = d3d12RootSignatureDesc.pParameters[param];
		descriptorTableSize.push_back(0);

		if (rootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
		{
			assert(rootParam.DescriptorTable.pDescriptorRanges != nullptr);

			hashCode = Utility::HashState(rootParam.DescriptorTable.pDescriptorRanges,
				rootParam.DescriptorTable.NumDescriptorRanges, hashCode);

			// We keep track of sampler descriptor tables separately from CBV_SRV_UAV descriptor tables
			if (rootParam.DescriptorTable.pDescriptorRanges->RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
			{
				samplerTableBitmap |= (1 << param);
			}
			else
			{
				descriptorTableBitmap |= (1 << param);
			}

			for (uint32_t tableRange = 0; tableRange < rootParam.DescriptorTable.NumDescriptorRanges; ++tableRange)
			{
				descriptorTableSize[param] += rootParam.DescriptorTable.pDescriptorRanges[tableRange].NumDescriptors;
			}
		}
		else
		{
			hashCode = Utility::HashState(&rootParam, 1, hashCode);
		}
	}

	ID3D12RootSignature** rootSignatureRef = nullptr;
	bool firstCompile = false;
	{
		lock_guard<mutex> lock(m_rootSignatureMutex);

		auto iter = m_rootSignatureHashMap.find(hashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == m_rootSignatureHashMap.end())
		{
			rootSignatureRef = m_rootSignatureHashMap[hashCode].addressof();
			firstCompile = true;
		}
		else
		{
			rootSignatureRef = iter->second.addressof();
		}
	}

	ID3D12RootSignature* pRootSignature = nullptr;

	if (firstCompile)
	{
		wil::com_ptr<ID3DBlob> pOutBlob, pErrorBlob;

		HRESULT hr = S_OK;
		hr = D3D12SerializeRootSignature(&d3d12RootSignatureDesc,D3D_ROOT_SIGNATURE_VERSION_1, &pOutBlob, &pErrorBlob);
		if (hr != S_OK)
		{
			LogError(LogDirectX) << "Error compiling root signature, HRESULT  0x" << std::hex << std::setw(8) << hr << endl;
			if (pErrorBlob)
			{
				LogError(LogDirectX) << (const char*)pErrorBlob->GetBufferPointer() << endl;
			}
			return nullptr;
		}

		assert_succeeded(m_device->CreateRootSignature(0, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(),
			IID_PPV_ARGS(&pRootSignature)));

		SetDebugName(pRootSignature, rootSignatureDesc.name);

		m_rootSignatureHashMap[hashCode].attach(pRootSignature);
		assert(*rootSignatureRef == pRootSignature);
	}
	else
	{
		while (rootSignatureRef == nullptr)
		{
			this_thread::yield();
		}
		pRootSignature = *rootSignatureRef;
	}

	auto rootSignature = std::make_shared<RootSignature>();

	rootSignature->m_device = this;
	rootSignature->m_desc = rootSignatureDesc;
	rootSignature->m_rootSignature = pRootSignature;
	rootSignature->m_descriptorTableBitmap = descriptorTableBitmap;
	rootSignature->m_samplerTableBitmap = samplerTableBitmap;
	rootSignature->m_descriptorTableSizes = descriptorTableSize;

	return rootSignature;
}


Luna::GraphicsPipelinePtr Device::CreateGraphicsPipeline(const GraphicsPipelineDesc& pipelineDesc)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3d12PipelineDesc{};
	d3d12PipelineDesc.NodeMask = 1;
	d3d12PipelineDesc.SampleMask = pipelineDesc.sampleMask;
	d3d12PipelineDesc.InputLayout.NumElements = 0;

	// Blend state
	d3d12PipelineDesc.BlendState.AlphaToCoverageEnable = pipelineDesc.blendState.alphaToCoverageEnable ? TRUE : FALSE;
	d3d12PipelineDesc.BlendState.IndependentBlendEnable = pipelineDesc.blendState.independentBlendEnable ? TRUE : FALSE;

	for (uint32_t i = 0; i < 8; ++i)
	{
		auto& rtDesc = d3d12PipelineDesc.BlendState.RenderTarget[i];
		const auto& renderTargetBlend = pipelineDesc.blendState.renderTargetBlend[i];

		rtDesc.BlendEnable = renderTargetBlend.blendEnable ? TRUE : FALSE;
		rtDesc.LogicOpEnable = renderTargetBlend.logicOpEnable ? TRUE : FALSE;
		rtDesc.SrcBlend = BlendToDX12(renderTargetBlend.srcBlend);
		rtDesc.DestBlend = BlendToDX12(renderTargetBlend.dstBlend);
		rtDesc.BlendOp = BlendOpToDX12(renderTargetBlend.blendOp);
		rtDesc.SrcBlendAlpha = BlendToDX12(renderTargetBlend.srcBlendAlpha);
		rtDesc.DestBlendAlpha = BlendToDX12(renderTargetBlend.dstBlendAlpha);
		rtDesc.BlendOpAlpha = BlendOpToDX12(renderTargetBlend.blendOpAlpha);
		rtDesc.LogicOp = LogicOpToDX12(renderTargetBlend.logicOp);
		rtDesc.RenderTargetWriteMask = ColorWriteToDX12(renderTargetBlend.writeMask);
	}

	// Rasterizer state
	const auto& rasterizerState = pipelineDesc.rasterizerState;
	d3d12PipelineDesc.RasterizerState.FillMode = FillModeToDX12(rasterizerState.fillMode);
	d3d12PipelineDesc.RasterizerState.CullMode = CullModeToDX12(rasterizerState.cullMode);
	d3d12PipelineDesc.RasterizerState.FrontCounterClockwise = rasterizerState.frontCounterClockwise ? TRUE : FALSE;
	d3d12PipelineDesc.RasterizerState.DepthBias = rasterizerState.depthBias;
	d3d12PipelineDesc.RasterizerState.DepthBiasClamp = rasterizerState.depthBiasClamp;
	d3d12PipelineDesc.RasterizerState.SlopeScaledDepthBias = rasterizerState.slopeScaledDepthBias;
	d3d12PipelineDesc.RasterizerState.DepthClipEnable = rasterizerState.depthClipEnable ? TRUE : FALSE;
	d3d12PipelineDesc.RasterizerState.MultisampleEnable = rasterizerState.multisampleEnable ? TRUE : FALSE;
	d3d12PipelineDesc.RasterizerState.AntialiasedLineEnable = rasterizerState.antialiasedLineEnable ? TRUE : FALSE;
	d3d12PipelineDesc.RasterizerState.ForcedSampleCount = rasterizerState.forcedSampleCount;
	d3d12PipelineDesc.RasterizerState.ConservativeRaster =
		rasterizerState.conservativeRasterizationEnable ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	// Depth-stencil state
	const auto& depthStencilState = pipelineDesc.depthStencilState;
	d3d12PipelineDesc.DepthStencilState.DepthEnable = depthStencilState.depthEnable ? TRUE : FALSE;
	d3d12PipelineDesc.DepthStencilState.DepthWriteMask = DepthWriteToDX12(depthStencilState.depthWriteMask);
	d3d12PipelineDesc.DepthStencilState.DepthFunc = ComparisonFuncToDX12(depthStencilState.depthFunc);
	d3d12PipelineDesc.DepthStencilState.StencilEnable = depthStencilState.stencilEnable ? TRUE : FALSE;
	d3d12PipelineDesc.DepthStencilState.StencilReadMask = depthStencilState.stencilReadMask;
	d3d12PipelineDesc.DepthStencilState.StencilWriteMask = depthStencilState.stencilWriteMask;
	d3d12PipelineDesc.DepthStencilState.FrontFace.StencilFailOp = StencilOpToDX12(depthStencilState.frontFace.stencilFailOp);
	d3d12PipelineDesc.DepthStencilState.FrontFace.StencilDepthFailOp = StencilOpToDX12(depthStencilState.frontFace.stencilDepthFailOp);
	d3d12PipelineDesc.DepthStencilState.FrontFace.StencilPassOp = StencilOpToDX12(depthStencilState.frontFace.stencilPassOp);
	d3d12PipelineDesc.DepthStencilState.FrontFace.StencilFunc = ComparisonFuncToDX12(depthStencilState.frontFace.stencilFunc);
	d3d12PipelineDesc.DepthStencilState.BackFace.StencilFailOp = StencilOpToDX12(depthStencilState.backFace.stencilFailOp);
	d3d12PipelineDesc.DepthStencilState.BackFace.StencilDepthFailOp = StencilOpToDX12(depthStencilState.backFace.stencilDepthFailOp);
	d3d12PipelineDesc.DepthStencilState.BackFace.StencilPassOp = StencilOpToDX12(depthStencilState.backFace.stencilPassOp);
	d3d12PipelineDesc.DepthStencilState.BackFace.StencilFunc = ComparisonFuncToDX12(depthStencilState.backFace.stencilFunc);

	// Primitive topology & primitive restart
	d3d12PipelineDesc.PrimitiveTopologyType = PrimitiveTopologyToPrimitiveTopologyTypeDX12(pipelineDesc.topology);
	d3d12PipelineDesc.IBStripCutValue = IndexBufferStripCutValueToDX12(pipelineDesc.indexBufferStripCut);

	// Render target formats
	const uint32_t numRtvs = (uint32_t)pipelineDesc.rtvFormats.size();
	const uint32_t maxRenderTargets = 8;
	for (uint32_t i = 0; i < numRtvs; ++i)
	{
		d3d12PipelineDesc.RTVFormats[i] = FormatToDxgi(pipelineDesc.rtvFormats[i]).rtvFormat;
	}
	for (uint32_t i = numRtvs; i < maxRenderTargets; ++i)
	{
		d3d12PipelineDesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
	}
	d3d12PipelineDesc.NumRenderTargets = numRtvs;
	d3d12PipelineDesc.DSVFormat = GetDSVFormat(FormatToDxgi(pipelineDesc.dsvFormat).resourceFormat);
	d3d12PipelineDesc.SampleDesc.Count = pipelineDesc.msaaCount;
	d3d12PipelineDesc.SampleDesc.Quality = 0; // TODO Rework this to enable quality levels in DX12

	// Input layout
	d3d12PipelineDesc.InputLayout.NumElements = (UINT)pipelineDesc.vertexElements.size();
	unique_ptr<const D3D12_INPUT_ELEMENT_DESC> d3dElements;

	if (d3d12PipelineDesc.InputLayout.NumElements > 0)
	{
		D3D12_INPUT_ELEMENT_DESC* newD3DElements = (D3D12_INPUT_ELEMENT_DESC*)malloc(sizeof(D3D12_INPUT_ELEMENT_DESC) * d3d12PipelineDesc.InputLayout.NumElements);

		const auto& vertexElements = pipelineDesc.vertexElements;

		for (uint32_t i = 0; i < d3d12PipelineDesc.InputLayout.NumElements; ++i)
		{
			newD3DElements[i].AlignedByteOffset = vertexElements[i].alignedByteOffset;
			newD3DElements[i].Format = FormatToDxgi(vertexElements[i].format).srvFormat;
			newD3DElements[i].InputSlot = vertexElements[i].inputSlot;
			newD3DElements[i].InputSlotClass = InputClassificationToDX12(vertexElements[i].inputClassification);
			newD3DElements[i].InstanceDataStepRate = vertexElements[i].instanceDataStepRate;
			newD3DElements[i].SemanticIndex = vertexElements[i].semanticIndex;
			newD3DElements[i].SemanticName = vertexElements[i].semanticName;
		}

		d3dElements.reset((const D3D12_INPUT_ELEMENT_DESC*)newD3DElements);
	}

	// Shaders
	if (pipelineDesc.vertexShader)
	{
		Shader* vertexShader = LoadShader(ShaderType::Vertex, pipelineDesc.vertexShader);
		assert(vertexShader);
		d3d12PipelineDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader->GetByteCode(), vertexShader->GetByteCodeSize());
	}

	if (pipelineDesc.pixelShader)
	{
		Shader* pixelShader = LoadShader(ShaderType::Pixel, pipelineDesc.pixelShader);
		assert(pixelShader);
		d3d12PipelineDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader->GetByteCode(), pixelShader->GetByteCodeSize());
	}

	if (pipelineDesc.geometryShader)
	{
		Shader* geometryShader = LoadShader(ShaderType::Geometry, pipelineDesc.geometryShader);
		assert(geometryShader);
		d3d12PipelineDesc.GS = CD3DX12_SHADER_BYTECODE(geometryShader->GetByteCode(), geometryShader->GetByteCodeSize());
	}

	if (pipelineDesc.hullShader)
	{
		Shader* hullShader = LoadShader(ShaderType::Hull, pipelineDesc.hullShader);
		assert(hullShader);
		d3d12PipelineDesc.HS = CD3DX12_SHADER_BYTECODE(hullShader->GetByteCode(), hullShader->GetByteCodeSize());
	}

	if (pipelineDesc.domainShader)
	{
		Shader* domainShader = LoadShader(ShaderType::Domain, pipelineDesc.domainShader);
		assert(domainShader);
		d3d12PipelineDesc.DS = CD3DX12_SHADER_BYTECODE(domainShader->GetByteCode(), domainShader->GetByteCodeSize());
	}

	// Get the root signature from the desc
	auto rootSignature = (RootSignature*)pipelineDesc.rootSignature.get();
	if (rootSignature)
	{
		d3d12PipelineDesc.pRootSignature = rootSignature->GetRootSignature();
	}
	assert(d3d12PipelineDesc.pRootSignature != nullptr);

	d3d12PipelineDesc.InputLayout.pInputElementDescs = nullptr;

	size_t hashCode = Utility::HashState(&d3d12PipelineDesc);
	hashCode = Utility::HashState(d3dElements.get(), d3d12PipelineDesc.InputLayout.NumElements, hashCode);

	d3d12PipelineDesc.InputLayout.pInputElementDescs = d3dElements.get();

	// Find or create graphics pipeline state object
	ID3D12PipelineState** pipelineStateRef = nullptr;
	bool firstCompile = false;
	{
		lock_guard<mutex> lock(m_graphicsPipelineStateMutex);

		auto iter = m_graphicsPipelineStateHashMap.find(hashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == m_graphicsPipelineStateHashMap.end())
		{
			pipelineStateRef = m_graphicsPipelineStateHashMap[hashCode].addressof();
			firstCompile = true;
		}
		else
		{
			pipelineStateRef = iter->second.addressof();
		}
	}

	ID3D12PipelineState* pPipelineState = nullptr;

	if (firstCompile)
	{
		wil::com_ptr<ID3DBlob> pOutBlob, pErrorBlob;

		HRESULT res = m_device->CreateGraphicsPipelineState(&d3d12PipelineDesc, IID_PPV_ARGS(&pPipelineState));
		ThrowIfFailed(res);

		SetDebugName(pPipelineState, pipelineDesc.name);

		m_graphicsPipelineStateHashMap[hashCode].attach(pPipelineState);
		assert(*pipelineStateRef == pPipelineState);
	}
	else
	{
		while (pipelineStateRef == nullptr)
		{
			this_thread::yield();
		}
		pPipelineState = *pipelineStateRef;
	}

	auto graphicsPipeline = std::make_shared<GraphicsPipeline>();

	graphicsPipeline->m_device = this;
	graphicsPipeline->m_desc = pipelineDesc;
	graphicsPipeline->m_pipelineState = pPipelineState;

	return graphicsPipeline;
}


Luna::ComputePipelinePtr Device::CreateComputePipeline(const ComputePipelineDesc& pipelineDesc)
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC d3d12PipelineDesc{};
	d3d12PipelineDesc.NodeMask = 1;
	
	Shader* computeShader = LoadShader(ShaderType::Compute, pipelineDesc.computeShader);
	assert(computeShader);
	d3d12PipelineDesc.CS = CD3DX12_SHADER_BYTECODE(computeShader->GetByteCode(), computeShader->GetByteCodeSize());

	// Get the root signature from the desc
	auto rootSignature = (RootSignature*)pipelineDesc.rootSignature.get();
	if (rootSignature)
	{
		d3d12PipelineDesc.pRootSignature = rootSignature->GetRootSignature();
	}
	assert(d3d12PipelineDesc.pRootSignature != nullptr);

	size_t hashCode = Utility::HashState(&d3d12PipelineDesc);

	// Find or create compute pipeline state object
	ID3D12PipelineState** pipelineStateRef = nullptr;
	bool firstCompile = false;
	{
		lock_guard<mutex> lock(m_computePipelineStateMutex);

		auto iter = m_computePipelineStateHashMap.find(hashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == m_computePipelineStateHashMap.end())
		{
			pipelineStateRef = m_computePipelineStateHashMap[hashCode].addressof();
			firstCompile = true;
		}
		else
		{
			pipelineStateRef = iter->second.addressof();
		}
	}

	ID3D12PipelineState* pPipelineState = nullptr;

	if (firstCompile)
	{
		wil::com_ptr<ID3DBlob> pOutBlob, pErrorBlob;

		HRESULT res = m_device->CreateComputePipelineState(&d3d12PipelineDesc, IID_PPV_ARGS(&pPipelineState));
		ThrowIfFailed(res);

		SetDebugName(pPipelineState, pipelineDesc.name);

		m_computePipelineStateHashMap[hashCode].attach(pPipelineState);
		assert(*pipelineStateRef == pPipelineState);
	}
	else
	{
		while (pipelineStateRef == nullptr)
		{
			this_thread::yield();
		}
		pPipelineState = *pipelineStateRef;
	}

	auto computePipeline = std::make_shared<ComputePipeline>();

	computePipeline->m_device = this;
	computePipeline->m_desc = pipelineDesc;
	computePipeline->m_pipelineState = pPipelineState;

	return computePipeline;
}


QueryHeapPtr Device::CreateQueryHeap(const QueryHeapDesc& queryHeapDesc)
{
	D3D12_QUERY_HEAP_DESC d3d12Desc{
		.Type		= QueryHeapTypeToDX12(queryHeapDesc.type),
		.Count		= queryHeapDesc.queryCount,
		.NodeMask	= 0
	};

	wil::com_ptr<ID3D12QueryHeap> pQueryHeap;
	HRESULT res = m_device->CreateQueryHeap(&d3d12Desc, IID_PPV_ARGS(&pQueryHeap));
	ThrowIfFailed(res);

	auto queryHeap = make_shared<QueryHeap>();
	queryHeap->m_desc = queryHeapDesc;
	queryHeap->m_heap = pQueryHeap;
	return queryHeap;
}


DescriptorSetPtr Device::CreateDescriptorSet(const DescriptorSetDesc& descriptorSetDesc)
{
	auto descriptorSet = std::make_shared<DescriptorSet>();

	descriptorSet->m_device = this;
	for (uint32_t i = 0; i < MaxDescriptorsPerTable; ++i)
	{
		descriptorSet->m_descriptors[i].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}
	descriptorSet->m_descriptorHandle = descriptorSetDesc.descriptorHandle;
	descriptorSet->m_numDescriptors = descriptorSetDesc.numDescriptors;
	descriptorSet->m_isSamplerTable = descriptorSetDesc.isSamplerTable;
	descriptorSet->m_isRootBuffer = descriptorSetDesc.isRootBuffer;

	return descriptorSet;
}


SamplerPtr Device::CreateSampler(const SamplerDesc& samplerDesc)
{
	D3D12_SAMPLER_DESC d3d12SamplerDesc{
		.Filter				= TextureFilterToDX12(samplerDesc.filter),
		.AddressU			= TextureAddressToDX12(samplerDesc.addressU),
		.AddressV			= TextureAddressToDX12(samplerDesc.addressV),
		.AddressW			= TextureAddressToDX12(samplerDesc.addressW),
		.MipLODBias			= samplerDesc.mipLODBias,
		.MaxAnisotropy		= samplerDesc.maxAnisotropy,
		.ComparisonFunc		= ComparisonFuncToDX12(samplerDesc.comparisonFunc),
		.BorderColor	= 
			{ samplerDesc.borderColor.R(), samplerDesc.borderColor.G(), samplerDesc.borderColor.B(), samplerDesc.borderColor.A() },
		.MinLOD				= { samplerDesc.minLOD },
		.MaxLOD				= { samplerDesc.maxLOD }
	};

	D3D12_CPU_DESCRIPTOR_HANDLE samplerHandle;

	const size_t hashValue = Utility::HashState(&d3d12SamplerDesc);

	std::lock_guard lock(m_samplerMutex);

	auto iter = m_samplerMap.find(hashValue);
	if (iter != m_samplerMap.end())
	{
		samplerHandle = iter->second;
	}
	else
	{
		samplerHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		m_device->CreateSampler(&d3d12SamplerDesc, samplerHandle);

		m_samplerMap[hashValue] = samplerHandle;
	}

	auto samplerPtr = std::make_shared<Sampler>();

	samplerPtr->m_device = this;
	samplerPtr->m_samplerHandle = samplerHandle;

	return samplerPtr;
}


TexturePtr Device::CreateTexture1D(const TextureDesc& textureDesc)
{
	return CreateTextureSimple(TextureDimension::Texture1D, textureDesc);
}


TexturePtr Device::CreateTexture2D(const TextureDesc& textureDesc)
{
	return CreateTextureSimple(TextureDimension::Texture2D, textureDesc);
}


TexturePtr Device::CreateTexture3D(const TextureDesc& textureDesc)
{
	return CreateTextureSimple(TextureDimension::Texture3D, textureDesc);
}


ITexture* Device::CreateUninitializedTexture(const std::string& name, const std::string& mapKey)
{
	Texture* tex = new Texture();
	tex->m_name = name;
	tex->m_mapKey = mapKey;
	return tex;
}


bool Device::InitializeTexture(ITexture* texture, const TextureInitializer& texInit)
{
	Texture* texture12 = (Texture*)texture;
	assert(texture12 != nullptr);

	const ResourceType type = TextureDimensionToResourceType(texInit.dimension);
	const bool isCubemap = HasAnyFlag(type, ResourceType::TextureCube_Type);
	uint32_t effectiveArraySize = isCubemap ? texInit.arraySizeOrDepth / 6 : texInit.arraySizeOrDepth;

	texture12->m_device = this;
	texture12->m_type = type;
	texture12->m_usageState = ResourceState::CopyDest;
	texture12->m_width = texInit.width;
	texture12->m_height = texInit.height;
	texture12->m_arraySizeOrDepth = effectiveArraySize;
	texture12->m_numMips = texInit.numMips;
	texture12->m_numSamples = 1;
	texture12->m_planeCount = GetFormatPlaneCount(FormatToDxgi(texInit.format).resourceFormat);
	texture12->m_format = texInit.format;
	texture12->m_dimension = texInit.dimension;

	// TODO: Allocate this with D3D12MA

	D3D12_RESOURCE_DESC texDesc{
		.Dimension			= GetResourceDimension(texture12->m_type),
		.Width				= texture12->m_width,
		.Height				= texture12->m_height,
		.DepthOrArraySize	= static_cast<UINT16>(texInit.arraySizeOrDepth),
		.MipLevels			= (UINT16)texture12->m_numMips,
		.Format				= FormatToDxgi(texture12->m_format).resourceFormat,
		.SampleDesc			= { .Count = 1, .Quality = 0 },
		.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN,
		.Flags				= D3D12_RESOURCE_FLAG_NONE
	};
	
	D3D12_HEAP_PROPERTIES heapProps{
		.Type					= D3D12_HEAP_TYPE_DEFAULT,
		.CPUPageProperty		= D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		.MemoryPoolPreference	= D3D12_MEMORY_POOL_UNKNOWN,
		.CreationNodeMask		= 1,
		.VisibleNodeMask		= 1
	};
	
	ID3D12Resource* resource = nullptr;
	assert_succeeded(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
		ResourceStateToDX12(texture12->m_usageState), nullptr, IID_PPV_ARGS(&resource)));

	texture12->m_resource.attach(resource);

	SetDebugName(texture12->GetResource(), texture12->m_name);

	// Copy initial data
	TexturePtr temp = texture;
	CommandContext::InitializeTexture(temp, texInit);

	// Create SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = GetSRVDimension(texture12->GetResourceType());
	srvDesc.Format = FormatToDxgi(texture12->GetFormat()).srvFormat;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	switch (texture12->GetResourceType())
	{
	case ResourceType::Texture1D:
		srvDesc.Texture1D.MipLevels = texture12->GetNumMips();
		break;
	case ResourceType::Texture1D_Array:
		srvDesc.Texture1DArray.MipLevels = texture12->GetNumMips();
		srvDesc.Texture1DArray.ArraySize = texture12->GetArraySize();
		break;
	case ResourceType::Texture2D:
	case ResourceType::Texture2DMS:
		srvDesc.Texture2D.MipLevels = texture12->GetNumMips();
		break;
	case ResourceType::Texture2D_Array:
	case ResourceType::Texture2DMS_Array:
		srvDesc.Texture2DArray.MipLevels = texture12->GetNumMips();
		srvDesc.Texture2DArray.ArraySize = texture12->GetArraySize();
		break;
	case ResourceType::TextureCube:
		srvDesc.TextureCube.MipLevels = texture12->GetNumMips();
		break;
	case ResourceType::TextureCube_Array:
		srvDesc.TextureCubeArray.MipLevels = texture12->GetNumMips();
		srvDesc.TextureCubeArray.NumCubes = texture12->GetArraySize();
		break;
	case ResourceType::Texture3D:
		srvDesc.Texture3D.MipLevels = texture12->GetNumMips();
		break;

	default:
		assert(false);
		return false;
	}

	if (texture12->m_srvHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
	{
		texture12->m_srvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	m_device->CreateShaderResourceView(texture12->GetResource(), &srvDesc, texture12->m_srvHandle);

	return true;
}


ColorBufferPtr Device::CreateColorBufferFromSwapChain(IDXGISwapChain* swapChain, uint32_t imageIndex)
{
	wil::com_ptr<ID3D12Resource> displayPlane;
	assert_succeeded(swapChain->GetBuffer(imageIndex, IID_PPV_ARGS(&displayPlane)));

	const std::string name = std::format("Primary SwapChain Image {}", imageIndex);
	SetDebugName(displayPlane.get(), name);

	auto rtvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_device->CreateRenderTargetView(displayPlane.get(), nullptr, rtvHandle);

	auto srvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_device->CreateShaderResourceView(displayPlane.get(), nullptr, srvHandle);

	D3D12_RESOURCE_DESC resourceDesc = displayPlane->GetDesc();
	const uint8_t planeCount = GetFormatPlaneCount(resourceDesc.Format);

	auto colorBuffer = std::make_shared<Luna::DX12::ColorBuffer>();

	colorBuffer->m_device = this;
	colorBuffer->m_type = ResourceType::Texture2D;
	colorBuffer->m_width = resourceDesc.Width;
	colorBuffer->m_height = resourceDesc.Height;
	colorBuffer->m_arraySizeOrDepth = resourceDesc.DepthOrArraySize;
	colorBuffer->m_numMips = 1;
	colorBuffer->m_numSamples = resourceDesc.SampleDesc.Count;
	colorBuffer->m_planeCount = planeCount;
	colorBuffer->m_format = DxgiToFormat(resourceDesc.Format);
	colorBuffer->m_dimension = ResourceTypeToTextureDimension(colorBuffer->m_type);
	colorBuffer->m_clearColor = DirectX::Colors::Black;
	colorBuffer->m_resource = displayPlane;
	colorBuffer->m_srvHandle = srvHandle;
	colorBuffer->m_rtvHandle = rtvHandle;
	
	return colorBuffer;
}


wil::com_ptr<D3D12MA::Allocation> Device::AllocateBuffer(const GpuBufferDesc& gpuBufferDesc) const
{
	const UINT64 bufferSize = gpuBufferDesc.elementSize * gpuBufferDesc.elementCount;

	D3D12_HEAP_TYPE heapType = GetHeapType(gpuBufferDesc.memoryAccess);
	D3D12_RESOURCE_FLAGS flags = (gpuBufferDesc.bAllowUnorderedAccess || IsUnorderedAccessType(gpuBufferDesc.resourceType))
		? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
		: D3D12_RESOURCE_FLAG_NONE;


	D3D12_RESOURCE_DESC resourceDesc{
		.Dimension			= D3D12_RESOURCE_DIMENSION_BUFFER,
		.Alignment			= 0,
		.Width				= bufferSize,
		.Height				= 1,
		.DepthOrArraySize	= 1,
		.MipLevels			= 1,
		.Format				= FormatToDxgi(gpuBufferDesc.format).resourceFormat,
		.SampleDesc		= { .Count = 1, .Quality = 0 },
		.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags				= flags
	};

	auto allocationDesc = D3D12MA::ALLOCATION_DESC{
		.HeapType = heapType
	};

	wil::com_ptr<D3D12MA::Allocation> allocation;
	HRESULT hr = m_allocator->CreateResource(
		&allocationDesc,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		NULL,
		&allocation,
		IID_NULL, NULL);

	return allocation;
}


TexturePtr Device::CreateTextureSimple(TextureDimension dimension, const TextureDesc& textureDesc)
{
	const size_t height = dimension == TextureDimension::Texture1D ? 1 : textureDesc.height;
	const size_t depth = dimension == TextureDimension::Texture3D ? textureDesc.depth : 1;

	assert(textureDesc.dataSize != 0);
	assert(textureDesc.data != nullptr);

	TextureInitializer texInit{ 
		.format				= textureDesc.format, 
		.dimension			= dimension,
		.width				= textureDesc.width,
		.height				= (uint32_t)height,
		.arraySizeOrDepth	= (uint32_t)depth,
		.numMips			= textureDesc.numMips
	};
	texInit.subResourceData.push_back(TextureSubresourceData{});

	size_t skipMip = 0;
	FillTextureInitializer(
		textureDesc.width,
		height,
		depth,
		textureDesc.numMips,
		1, // arraySize
		textureDesc.format,
		0, // maxSize
		textureDesc.dataSize,
		textureDesc.data,
		skipMip,
		texInit);

	TexturePtr texture = CreateUninitializedTexture(textureDesc.name, textureDesc.name);
	InitializeTexture(texture.Get(), texInit);

	return texture;
}


Device* GetD3D12Device()
{
	assert(g_d3d12Device != nullptr);

	return g_d3d12Device;
}

} // namespace Luna::DX12