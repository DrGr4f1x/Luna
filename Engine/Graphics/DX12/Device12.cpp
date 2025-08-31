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
#include "DeviceManager12.h"
#include "GpuBuffer12.h"
#include "PipelineState12.h"
#include "PipelineStateUtil12.h"
#include "QueryHeap12.h"
#include "RootSignature12.h"
#include "Sampler12.h"
#include "Shader12.h"
#include "Texture12.h"

using namespace std;


namespace Luna::DX12
{

static Device* g_d3d12Device{ nullptr };


Device::Device(ID3D12Device* device, D3D12MA::Allocator* allocator)
	: m_device{ device }
	, m_allocator{ allocator }
{
	m_freeDescriptors.resize(D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES, vector<DescriptorHandle2>());

	m_device2 = m_device.query<ID3D12Device2>();

	assert(g_d3d12Device == nullptr);
	g_d3d12Device = this;
}


Luna::ColorBufferPtr Device::CreateColorBuffer(const ColorBufferDesc& colorBufferDesc)
{
	// Create color buffer
	auto colorBuffer = std::make_shared<Luna::DX12::ColorBuffer>(this);
	colorBuffer->m_type = colorBufferDesc.resourceType;
	colorBuffer->m_width = colorBufferDesc.width;
	colorBuffer->m_height = colorBufferDesc.height;
	colorBuffer->m_arraySizeOrDepth = colorBufferDesc.arraySizeOrDepth;
	colorBuffer->m_numSamples = colorBufferDesc.numSamples;
	colorBuffer->m_planeCount = GetFormatPlaneCount(FormatToDxgi(colorBufferDesc.format).resourceFormat);
	colorBuffer->m_format = colorBufferDesc.format;
	colorBuffer->m_dimension = ResourceTypeToTextureDimension(colorBuffer->m_type);
	colorBuffer->m_clearColor = colorBufferDesc.clearColor;
	
	// Create resource
	auto numMips = colorBufferDesc.numMips == 0 ? ComputeNumMips(colorBufferDesc.width, colorBufferDesc.height) : colorBufferDesc.numMips;
	auto flags = CombineResourceFlags(colorBufferDesc.numSamples);

	colorBuffer->m_numMips = numMips;

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

	colorBuffer->m_resource = resource;

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

	// Create SRV and RTV descriptors
	colorBuffer->m_srvDescriptor.CreateShaderResourceView(resource.get(), srvDesc);
	colorBuffer->m_rtvDescriptor.CreateRenderTargetView(resource.get(), rtvDesc);

	// Create the UAVs for each mip level (RWTexture2D)
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 12> uavHandles;
	for (auto& uavHandle : uavHandles)
	{
		uavHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	if (colorBufferDesc.numSamples == 1)
	{
		for (uint32_t i = 0; i < numMips; ++i)
		{
			colorBuffer->m_uavDescriptors[i].CreateUnorderedAccessView(resource.get(), uavDesc);

			uavDesc.Texture2D.MipSlice++;
		}
	}

	return colorBuffer;
}


Luna::DepthBufferPtr Device::CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc)
{
	auto depthBuffer = std::make_shared<Luna::DX12::DepthBuffer>(this);

	depthBuffer->m_type = depthBufferDesc.resourceType;
	depthBuffer->m_width = depthBufferDesc.width;
	depthBuffer->m_height = depthBufferDesc.height;
	depthBuffer->m_arraySizeOrDepth = depthBufferDesc.arraySizeOrDepth;
	depthBuffer->m_numMips = 1;
	depthBuffer->m_numSamples = depthBufferDesc.numSamples;
	depthBuffer->m_planeCount = GetFormatPlaneCount(FormatToDxgi(depthBufferDesc.format).resourceFormat);
	depthBuffer->m_format = depthBufferDesc.format;
	depthBuffer->m_dimension = ResourceTypeToTextureDimension(depthBuffer->m_type);
	depthBuffer->m_clearDepth = depthBufferDesc.clearDepth;
	depthBuffer->m_clearStencil = depthBufferDesc.clearStencil;

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

	depthBuffer->m_resource = resource;

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

	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	depthBuffer->m_dsvDescriptors[0].CreateDepthStencilView(resource.get(), dsvDesc);

	dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
	depthBuffer->m_dsvDescriptors[1].CreateDepthStencilView(resource.get(), dsvDesc);

	auto stencilReadFormat = GetStencilFormat(FormatToDxgi(depthBufferDesc.format).resourceFormat);
	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
	{
		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		depthBuffer->m_dsvDescriptors[2].CreateDepthStencilView(resource.get(), dsvDesc);

		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		depthBuffer->m_dsvDescriptors[3].CreateDepthStencilView(resource.get(), dsvDesc);
	}
	else
	{
		// dsvDescriptors 2 and 3 are the same as 0 and 1, respectively
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		depthBuffer->m_dsvDescriptors[2].CreateDepthStencilView(resource.get(), dsvDesc);

		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
		depthBuffer->m_dsvDescriptors[3].CreateDepthStencilView(resource.get(), dsvDesc);
	}

	if (depthBufferDesc.createShaderResources)
	{
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

		depthBuffer->m_depthSrvDescriptor.CreateShaderResourceView(resource.get(), srvDesc);

		if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
		{
			srvDesc.Format = stencilReadFormat;
			srvDesc.Texture2D.PlaneSlice = (srvDesc.Format == DXGI_FORMAT_X32_TYPELESS_G8X24_UINT) ? 1 : 0;
			
			depthBuffer->m_stencilSrvDescriptor.CreateShaderResourceView(resource.get(), srvDesc);
		}
	}

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

	// Create GpuBuffer
	auto gpuBuffer = std::make_shared<GpuBuffer>(this);
	gpuBuffer->m_type = gpuBufferDesc.resourceType;
	gpuBuffer->m_usageState = initialState;
	gpuBuffer->m_format = gpuBufferDesc.format;
	gpuBuffer->m_elementSize = gpuBufferDescIn.elementSize;
	gpuBuffer->m_elementCount = gpuBufferDesc.elementCount;
	gpuBuffer->m_bufferSize = gpuBuffer->m_elementCount * gpuBuffer->m_elementSize;
	gpuBuffer->m_isCpuWriteable = HasFlag(gpuBufferDesc.memoryAccess, MemoryAccess::CpuWrite);

	wil::com_ptr<D3D12MA::Allocation> allocation = AllocateBuffer(gpuBufferDesc);
	ID3D12Resource* pResource = allocation->GetResource();

	SetDebugName(pResource, gpuBufferDesc.name);

	gpuBuffer->m_allocation = allocation;

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

		gpuBuffer->m_srvDescriptor.CreateShaderResourceView(pResource, srvDesc);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{
			.Format				= DXGI_FORMAT_R32_TYPELESS,
			.ViewDimension		= D3D12_UAV_DIMENSION_BUFFER,
			.Buffer	= {
				.NumElements	= (uint32_t)bufferSize / 4,
				.Flags			= D3D12_BUFFER_UAV_FLAG_RAW
			}
		};

		gpuBuffer->m_uavDescriptor.CreateUnorderedAccessView(pResource, uavDesc);
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

		gpuBuffer->m_srvDescriptor.CreateShaderResourceView(pResource, srvDesc);

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

		gpuBuffer->m_uavDescriptor.CreateUnorderedAccessView(pResource, uavDesc);
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

		gpuBuffer->m_srvDescriptor.CreateShaderResourceView(pResource, srvDesc);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{
			.Format				= FormatToDxgi(gpuBufferDesc.format).resourceFormat,
			.ViewDimension		= D3D12_UAV_DIMENSION_BUFFER,
			.Buffer = {
				.NumElements	= (uint32_t)gpuBufferDesc.elementCount,
				.Flags			= D3D12_BUFFER_UAV_FLAG_NONE
			}
		};

		gpuBuffer->m_uavDescriptor.CreateUnorderedAccessView(pResource, uavDesc);
	}

	if (gpuBufferDesc.resourceType == ResourceType::ConstantBuffer)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{
			.BufferLocation		= pResource->GetGPUVirtualAddress(),
			.SizeInBytes		= (uint32_t)(gpuBufferDesc.elementCount * gpuBufferDesc.elementSize)
		};

		gpuBuffer->m_cbvDescriptor.CreateConstantBufferView(cbvDesc);
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

			gpuBuffer->m_srvDescriptor.CreateShaderResourceView(pResource, srvDesc);
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

			gpuBuffer->m_uavDescriptor.CreateUnorderedAccessView(pResource, uavDesc);
		}
	}

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
	std::vector<D3D12_ROOT_PARAMETER1> d3d12RootParameters;

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
	ShaderStage combinedShaderStages{};
	for (const auto& rootParameter : rootSignatureDesc.rootParameters)
	{
		combinedShaderStages |= rootParameter.shaderVisibility;

		if (rootParameter.parameterType == RootParameterType::RootConstants)
		{
			D3D12_ROOT_PARAMETER1& param = d3d12RootParameters.emplace_back();
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
			param.ShaderVisibility = ShaderStageToDX12(rootParameter.shaderVisibility);
			param.Constants.Num32BitValues = rootParameter.num32BitConstants;
			param.Constants.RegisterSpace = rootParameter.registerSpace;
			param.Constants.ShaderRegister = rootParameter.startRegister;
		}
		else if (IsRootDescriptorType(rootParameter.parameterType))
		{
			D3D12_ROOT_PARAMETER1& param = d3d12RootParameters.emplace_back();
			param.ParameterType = RootParameterTypeToDX12(rootParameter.parameterType);
			param.ShaderVisibility = ShaderStageToDX12(rootParameter.shaderVisibility);
			param.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;
			param.Descriptor.RegisterSpace = rootParameter.registerSpace;
			param.Descriptor.ShaderRegister = rootParameter.startRegister;
		}
		else if (rootParameter.parameterType == RootParameterType::Table)
		{
			D3D12_ROOT_PARAMETER1& param = d3d12RootParameters.emplace_back();
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			param.ShaderVisibility = ShaderStageToDX12(rootParameter.shaderVisibility);

			uint32_t currentRegister[] = { 0, 0, 0, 0 };

			const uint32_t numRanges = (uint32_t)rootParameter.table.size();
			param.DescriptorTable.NumDescriptorRanges = numRanges;
			D3D12_DESCRIPTOR_RANGE1* pRanges = new D3D12_DESCRIPTOR_RANGE1[numRanges];

			for (uint32_t i = 0; i < numRanges; ++i)
			{
				D3D12_DESCRIPTOR_RANGE1& d3d12Range = pRanges[i];
				D3D12_DESCRIPTOR_RANGE_FLAGS descriptorRangeFlags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

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

				// Set flags
				if (HasAnyFlag(range.flags, DescriptorRangeFlags::PartiallyBound | DescriptorRangeFlags::AllowUpdateAfterSet))
				{
					descriptorRangeFlags |= D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
				}
				if (range.descriptorType != DescriptorType::Sampler)
				{
					if (HasFlag(range.flags, DescriptorRangeFlags::AllowUpdateAfterSet))
					{
						descriptorRangeFlags |= D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE;
					}
					else
					{
						descriptorRangeFlags |= D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
					}
				}
				d3d12Range.Flags = descriptorRangeFlags;
			}
			param.DescriptorTable.pDescriptorRanges = pRanges;
		}
	}

	// Build static samplers
	vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
	uint32_t currentSamplerRegister = 0;
	for (const auto& staticSamplerDesc : rootSignatureDesc.staticSamplers)
	{
		combinedShaderStages |= staticSamplerDesc.shaderStage;

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

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC d3d12RootSignatureDesc{
		.Version			= D3D_ROOT_SIGNATURE_VERSION_1_1,
		.Desc_1_1 = {
			.NumParameters			= (uint32_t)d3d12RootParameters.size(),
			.pParameters			= d3d12RootParameters.data(),
			.NumStaticSamplers		= (uint32_t)staticSamplers.size(),
			.pStaticSamplers		= staticSamplers.data(),
			.Flags					= ShaderStageToRootSignatureFlags(combinedShaderStages)
		}
	};

	uint32_t descriptorTableBitmap{ 0 };
	uint32_t samplerTableBitmap{ 0 };
	std::vector<uint32_t> descriptorTableSize;
	descriptorTableSize.reserve(16);

	// Calculate hash
	size_t hashCode = 0;
	for (uint32_t param = 0; param < d3d12RootSignatureDesc.Desc_1_1.NumParameters; ++param)
	{
		const D3D12_ROOT_PARAMETER1& rootParam = d3d12RootSignatureDesc.Desc_1_1.pParameters[param];
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
		hr = D3D12SerializeVersionedRootSignature(&d3d12RootSignatureDesc, &pOutBlob, &pErrorBlob);
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
	GraphicsPipelineContext context{};
	FillGraphicsPipelineDesc(context, pipelineDesc);

#if defined(D3D12_SDK_VERSION) && (D3D12_SDK_VERSION >= 610)
	if (m_caps.features.rasterizerDesc2)
	{
		CD3DX12_PIPELINE_STATE_STREAM5 pipelineStream5{};
		FillGraphicsPipelineStateStream5(pipelineStream5, context.stateDesc, pipelineDesc);
		return CreateGraphicsPipelineStream<CD3DX12_PIPELINE_STATE_STREAM5>(pipelineStream5, context.hashCode, pipelineDesc);
	}
	else
	{
		CD3DX12_PIPELINE_STATE_STREAM4 pipelineStream4{};
		FillGraphicsPipelineStateStream4(pipelineStream4, context.stateDesc, pipelineDesc);
		return CreateGraphicsPipelineStream<CD3DX12_PIPELINE_STATE_STREAM4>(pipelineStream4, context.hashCode, pipelineDesc);
	}
#endif

#if defined(D3D12_SDK_VERSION) && (D3D12_SDK_VERSION >= 608)
	{
		CD3DX12_PIPELINE_STATE_STREAM4 pipelineStream4{};
		FillGraphicsPipelineStateStream4(pipelineStream4, context.stateDesc, pipelineDesc);
		return CreateGraphicsPipelineStream<CD3DX12_PIPELINE_STATE_STREAM4>(pipelineStream4, context.hashCode, pipelineDesc);
	}
#endif

#if defined(D3D12_SDK_VERSION) && (D3D12_SDK_VERSION >= 606)
	{
		CD3DX12_PIPELINE_STATE_STREAM3 pipelineStream3{};
		FillGraphicsPipelineStateStream3(pipelineStream3, context.stateDesc, pipelineDesc);
		return CreateGraphicsPipelineStream<CD3DX12_PIPELINE_STATE_STREAM3>(pipelineStream3, context.hashCode, pipelineDesc);
	}
#endif

	
	CD3DX12_PIPELINE_STATE_STREAM2 pipelineStream2{};
	FillGraphicsPipelineStateStream2(pipelineStream2, context.stateDesc, pipelineDesc);
	return CreateGraphicsPipelineStream<CD3DX12_PIPELINE_STATE_STREAM2>(pipelineStream2, context.hashCode, pipelineDesc);
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
	auto descriptorSet = std::make_shared<DescriptorSet>(this, descriptorSetDesc.rootParameter);

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

	shared_ptr<Sampler> sampler;

	const size_t hashValue = Utility::HashState(&d3d12SamplerDesc);

	std::lock_guard lock(m_samplerMutex);

	auto iter = m_samplerMap.find(hashValue);
	if (iter != m_samplerMap.end())
	{
		sampler = iter->second;
	}
	else
	{
		sampler = make_shared<Sampler>(this);
		sampler->m_samplerDescriptor.CreateSampler(d3d12SamplerDesc);

		m_samplerMap[hashValue] = sampler;
	}

	return sampler;
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
	Texture* tex = new Texture(this);
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

	texture12->m_srvDescriptor.CreateShaderResourceView(texture12->GetResource(), srvDesc);

	return true;
}


ColorBufferPtr Device::CreateColorBufferFromSwapChain(IDXGISwapChain* swapChain, uint32_t imageIndex)
{
	wil::com_ptr<ID3D12Resource> displayPlane;
	assert_succeeded(swapChain->GetBuffer(imageIndex, IID_PPV_ARGS(&displayPlane)));

	const std::string name = std::format("Primary SwapChain Image {}", imageIndex);
	SetDebugName(displayPlane.get(), name);

	D3D12_RESOURCE_DESC resourceDesc = displayPlane->GetDesc();
	const uint8_t planeCount = GetFormatPlaneCount(resourceDesc.Format);

	auto colorBuffer = std::make_shared<Luna::DX12::ColorBuffer>(this);
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

	colorBuffer->m_srvDescriptor.CreateShaderResourceView(displayPlane.get());
	colorBuffer->m_rtvDescriptor.CreateRenderTargetView(displayPlane.get());
	
	return colorBuffer;
}


void Device::FillCaps(const AdapterInfo& adapterInfo)
{
	HRESULT hr{};

	m_caps.adapterInfo = adapterInfo;
	m_caps.api = GraphicsApi::D3D12;

	D3D12_FEATURE_DATA_D3D12_OPTIONS options{};
	hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options) failed, result = {:#x}", hr);
	}
	m_caps.tiers.memory = options.ResourceHeapTier == D3D12_RESOURCE_HEAP_TIER_2 ? 1 : 0;

	D3D12_FEATURE_DATA_D3D12_OPTIONS1 options1{};
	hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &options1, sizeof(options1));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options1) failed, result = {:#x}", hr);
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS2 options2{};
	hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS2, &options2, sizeof(options2));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options2) failed, result = {:#x}", hr);
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS3 options3{};
	hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &options3, sizeof(options3));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options3) failed, result = {:#x}", hr);
	}
	m_caps.features.copyQueueTimestamp = options3.CopyQueueTimestampQueriesSupported;

	D3D12_FEATURE_DATA_D3D12_OPTIONS4 options4{};
	hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &options4, sizeof(options4));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options4) failed, result = {:#x}", hr);
	}

	// Windows 10 1809 (build 17763)
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5{};
	hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options5) failed, result = {:#x}", hr);
	}
	m_caps.features.rayTracing = options5.RaytracingTier != 0;
	m_caps.tiers.rayTracing = (uint8_t)(options5.RaytracingTier - D3D12_RAYTRACING_TIER_1_0 + 1);

	// Windows 10 1903 (build 18362)
	D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6{};
	hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options6) failed, result = {:#x}", hr);
	}
	m_caps.tiers.shadingRate = (uint8_t)options6.VariableShadingRateTier;
	m_caps.other.shadingRateAttachmentTileSize = (uint8_t)options6.ShadingRateImageTileSize;
	m_caps.features.additionalShadingRates = options6.AdditionalShadingRatesSupported;

	// Windows 10 2004 (build 19041)
	D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7{};
	hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options7) failed, result = {:#x}", hr);
	}
	m_caps.features.meshShader = options7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1;

#ifdef USE_AGILITY_SDK
	// Windows 11 21H2 (build 22000)
	D3D12_FEATURE_DATA_D3D12_OPTIONS8 options8{};
	hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS8, &options8, sizeof(options8));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options8) failed, result = {:#x}", hr);
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS9 options9{};
	hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS9, &options9, sizeof(options9));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options9) failed, result = {:#x}", hr);
	}
	m_caps.features.meshShaderPipelineStats = options9.MeshShaderPipelineStatsSupported;

	D3D12_FEATURE_DATA_D3D12_OPTIONS10 options10{};
	hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS10, &options10, sizeof(options10));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options10) failed, result = {:#x}", hr);
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS11 options11{};
	hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS11, &options11, sizeof(options11));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options11) failed, result = {:#x}", hr);
	}

	// Windows 11 22H2 (build 22621)
	D3D12_FEATURE_DATA_D3D12_OPTIONS12 options12{};
	hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &options12, sizeof(options12));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options12) failed, result = {:#x}", hr);
	}
	m_caps.features.enhancedBarriers = options12.EnhancedBarriersSupported;

	D3D12_FEATURE_DATA_D3D12_OPTIONS13 options13{};
	hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS13, &options13, sizeof(options13));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options13) failed, result = {:#x}", hr);
	}
	m_caps.memoryAlignment.uploadBufferTextureRow = options13.UnrestrictedBufferTextureCopyPitchSupported ? 1 : D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
	m_caps.memoryAlignment.uploadBufferTextureSlice = options13.UnrestrictedBufferTextureCopyPitchSupported ? 1 : D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
	m_caps.features.viewportOriginBottomLeft = options13.InvertedViewportHeightFlipsYSupported ? 1 : 0;

	// Agility SDK
	D3D12_FEATURE_DATA_D3D12_OPTIONS14 options14{};
	hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS14, &options14, sizeof(options14));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options14) failed, result = {:#x}", hr);
	}
	m_caps.features.independentFrontAndBackStencilReferenceAndMasks = options14.IndependentFrontAndBackStencilRefMaskSupported ? true : false;

	D3D12_FEATURE_DATA_D3D12_OPTIONS15 options15{};
	hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS15, &options15, sizeof(options15));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options15) failed, result = {:#x}", hr);
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS16 options16{};
	hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS16, &options16, sizeof(options16));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options16) failed, result = {:#x}", hr);
	}
	m_caps.memory.deviceUploadHeapSize = options16.GPUUploadHeapSupported ? m_caps.adapterInfo.dedicatedVideoMemory : 0;
	m_caps.features.dynamicDepthBias = options16.DynamicDepthBiasSupported;

	D3D12_FEATURE_DATA_D3D12_OPTIONS17 options17{};
	hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS17, &options17, sizeof(options17));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options17) failed, result = {:#x}", hr);
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS18 options18{};
	hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS18, &options18, sizeof(options18));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options18) failed, result = {:#x}", hr);
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS19 options19{};
	hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS19, &options19, sizeof(options19));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options19) failed, result = {:#x}", hr);
	}
	m_caps.features.rasterizerDesc2 = options19.RasterizerDesc2Supported;

	D3D12_FEATURE_DATA_D3D12_OPTIONS20 options20{};
	hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS20, &options20, sizeof(options20));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options20) failed, result = {:#x}", hr);
	}

	D3D12_FEATURE_DATA_D3D12_OPTIONS21 options21{};
	hr = m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS21, &options21, sizeof(options21));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport(options21) failed, result = {:#x}", hr);
	}
#else
	m_caps.memoryAlignment.uploadBufferTextureRow = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
	m_caps.memoryAlignment.uploadBufferTextureSlice = D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
#endif

	// Feature level
	const std::array<D3D_FEATURE_LEVEL, 5> levelsList = 
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_2,
	};

	D3D12_FEATURE_DATA_FEATURE_LEVELS levels{};
	levels.NumFeatureLevels = (uint32_t)levelsList.size();
	levels.pFeatureLevelsRequested = levelsList.data();
	hr = m_device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &levels, sizeof(levels));
	if (FAILED(hr))
	{
		LogWarning(LogDirectX) << format("ID3D12Device::CheckFeatureSupport((D3D12_FEATURE_FEATURE_LEVELS) failed, result = {:#x}", hr);
	}

	// Shader model
#if (D3D12_SDK_VERSION >= 6)
	D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { (D3D_SHADER_MODEL)D3D_HIGHEST_SHADER_MODEL };
#else
	D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = { (D3D_SHADER_MODEL)0x69 };
#endif
	for (; shaderModel.HighestShaderModel >= D3D_SHADER_MODEL_6_0; (*(uint32_t*)&shaderModel.HighestShaderModel)--) 
	{
		hr = m_device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel));
		if (SUCCEEDED(hr))
		{
			break;
		}
	}
	if (shaderModel.HighestShaderModel < D3D_SHADER_MODEL_6_0)
	{
		shaderModel.HighestShaderModel = D3D_SHADER_MODEL_5_1;
	}

	m_caps.shaderModel = (uint8_t)((shaderModel.HighestShaderModel / 0xF) * 10 + (shaderModel.HighestShaderModel & 0xF));

	// Viewport
	m_caps.viewport.maxNum = D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
	m_caps.viewport.boundsMin = D3D12_VIEWPORT_BOUNDS_MIN;
	m_caps.viewport.boundsMax = D3D12_VIEWPORT_BOUNDS_MAX;

	// Dimensions
	m_caps.dimensions.attachmentMaxDim = D3D12_REQ_RENDER_TO_BUFFER_WINDOW_WIDTH;
	m_caps.dimensions.attachmentLayerMaxNum = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
	m_caps.dimensions.texture1DMaxDim = D3D12_REQ_TEXTURE1D_U_DIMENSION;
	m_caps.dimensions.texture2DMaxDim = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;
	m_caps.dimensions.texture3DMaxDim = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
	m_caps.dimensions.textureCubeMaxDim = D3D12_REQ_TEXTURECUBE_DIMENSION;
	m_caps.dimensions.textureLayerMaxNum = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION;
	m_caps.dimensions.typedBufferMaxDim = 1 << D3D12_REQ_BUFFER_RESOURCE_TEXEL_COUNT_2_TO_EXP;

	// Precision
	m_caps.precision.viewportBits = D3D12_SUBPIXEL_FRACTIONAL_BIT_COUNT;
	m_caps.precision.subPixelBits = D3D12_SUBPIXEL_FRACTIONAL_BIT_COUNT;
	m_caps.precision.subTexelBits = D3D12_SUBTEXEL_FRACTIONAL_BIT_COUNT;
	m_caps.precision.mipmapBits = D3D12_MIP_LOD_FRACTIONAL_BIT_COUNT;

	// Memory
	m_caps.memory.allocationMaxNum = 0xFFFFFFFF;
	m_caps.memory.samplerAllocationMaxNum = D3D12_REQ_SAMPLER_OBJECT_COUNT_PER_DEVICE;
	m_caps.memory.constantBufferMaxRange = D3D12_REQ_IMMEDIATE_CONSTANT_BUFFER_ELEMENT_COUNT * 16;
	m_caps.memory.storageBufferMaxRange = 1 << D3D12_REQ_BUFFER_RESOURCE_TEXEL_COUNT_2_TO_EXP;
	m_caps.memory.bufferTextureGranularity = D3D12_SMALL_RESOURCE_PLACEMENT_ALIGNMENT;
	m_caps.memory.bufferMaxSize = D3D12_REQ_RESOURCE_SIZE_IN_MEGABYTES_EXPRESSION_C_TERM * 1024ull * 1024ull;

	// Memory alignment
	m_caps.memoryAlignment.shaderBindingTable = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
	m_caps.memoryAlignment.bufferShaderResourceOffset = D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT;
	m_caps.memoryAlignment.constantBufferOffset = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
	m_caps.memoryAlignment.scratchBufferOffset = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
	m_caps.memoryAlignment.accelerationStructureOffset = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
#if D3D12_HAS_OPACITY_MICROMAP
	m_caps.memoryAlignment.micromapOffset = D3D12_RAYTRACING_OPACITY_MICROMAP_ARRAY_BYTE_ALIGNMENT;
#endif

	// Pipeline layout
	m_caps.pipelineLayout.descriptorSetMaxNum = ROOT_SIGNATURE_DWORD_NUM / 1;
	m_caps.pipelineLayout.rootConstantMaxSize = sizeof(uint32_t) * ROOT_SIGNATURE_DWORD_NUM / 1;
	m_caps.pipelineLayout.rootDescriptorMaxNum = ROOT_SIGNATURE_DWORD_NUM / 2;

	// Descriptor set
	// https://learn.microsoft.com/en-us/windows/win32/direct3d12/hardware-support
	if (options.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_1) 
	{
		m_caps.descriptorSet.samplerMaxNum = 16;
		m_caps.descriptorSet.constantBufferMaxNum = 14;
		m_caps.descriptorSet.storageBufferMaxNum = levels.MaxSupportedFeatureLevel >= D3D_FEATURE_LEVEL_11_1 ? 64 : 8;
		m_caps.descriptorSet.textureMaxNum = 128;
	}
	else if (options.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_2) 
	{
		m_caps.descriptorSet.samplerMaxNum = 2048;
		m_caps.descriptorSet.constantBufferMaxNum = 14;
		m_caps.descriptorSet.storageBufferMaxNum = 64;
		m_caps.descriptorSet.textureMaxNum = D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2;
	}
	else 
	{
		m_caps.descriptorSet.samplerMaxNum = 2048;
		m_caps.descriptorSet.constantBufferMaxNum = D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2;
		m_caps.descriptorSet.storageBufferMaxNum = D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2;
		m_caps.descriptorSet.textureMaxNum = D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2;
	}
	m_caps.descriptorSet.storageTextureMaxNum = m_caps.descriptorSet.storageBufferMaxNum;

	m_caps.descriptorSet.updateAfterSet.samplerMaxNum = m_caps.descriptorSet.samplerMaxNum;
	m_caps.descriptorSet.updateAfterSet.constantBufferMaxNum = m_caps.descriptorSet.constantBufferMaxNum;
	m_caps.descriptorSet.updateAfterSet.storageBufferMaxNum = m_caps.descriptorSet.storageBufferMaxNum;
	m_caps.descriptorSet.updateAfterSet.textureMaxNum = m_caps.descriptorSet.textureMaxNum;
	m_caps.descriptorSet.updateAfterSet.storageTextureMaxNum = m_caps.descriptorSet.storageTextureMaxNum;

	m_caps.shaderStage.descriptorSamplerMaxNum = m_caps.descriptorSet.samplerMaxNum;
	m_caps.shaderStage.descriptorConstantBufferMaxNum = m_caps.descriptorSet.constantBufferMaxNum;
	m_caps.shaderStage.descriptorStorageBufferMaxNum = m_caps.descriptorSet.storageBufferMaxNum;
	m_caps.shaderStage.descriptorTextureMaxNum = m_caps.descriptorSet.textureMaxNum;
	m_caps.shaderStage.descriptorStorageTextureMaxNum = m_caps.descriptorSet.storageTextureMaxNum;
	m_caps.shaderStage.resourceMaxNum = m_caps.descriptorSet.textureMaxNum;

	m_caps.shaderStage.updateAfterSet.descriptorSamplerMaxNum = m_caps.shaderStage.descriptorSamplerMaxNum;
	m_caps.shaderStage.updateAfterSet.descriptorConstantBufferMaxNum = m_caps.shaderStage.descriptorConstantBufferMaxNum;
	m_caps.shaderStage.updateAfterSet.descriptorStorageBufferMaxNum = m_caps.shaderStage.descriptorStorageBufferMaxNum;
	m_caps.shaderStage.updateAfterSet.descriptorTextureMaxNum = m_caps.shaderStage.descriptorTextureMaxNum;
	m_caps.shaderStage.updateAfterSet.descriptorStorageTextureMaxNum = m_caps.shaderStage.descriptorStorageTextureMaxNum;
	m_caps.shaderStage.updateAfterSet.resourceMaxNum = m_caps.shaderStage.resourceMaxNum;

	m_caps.shaderStage.vertex.attributeMaxNum = D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
	m_caps.shaderStage.vertex.streamMaxNum = D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT;
	m_caps.shaderStage.vertex.outputComponentMaxNum = D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT * 4;

	m_caps.shaderStage.tesselationControl.generationMaxLevel = D3D12_HS_MAXTESSFACTOR_UPPER_BOUND;
	m_caps.shaderStage.tesselationControl.patchPointMaxNum = D3D12_IA_PATCH_MAX_CONTROL_POINT_COUNT;
	m_caps.shaderStage.tesselationControl.perVertexInputComponentMaxNum = D3D12_HS_CONTROL_POINT_PHASE_INPUT_REGISTER_COUNT * D3D12_HS_CONTROL_POINT_REGISTER_COMPONENTS;
	m_caps.shaderStage.tesselationControl.perVertexOutputComponentMaxNum = D3D12_HS_CONTROL_POINT_PHASE_OUTPUT_REGISTER_COUNT * D3D12_HS_CONTROL_POINT_REGISTER_COMPONENTS;
	m_caps.shaderStage.tesselationControl.perPatchOutputComponentMaxNum = D3D12_HS_OUTPUT_PATCH_CONSTANT_REGISTER_SCALAR_COMPONENTS;
	m_caps.shaderStage.tesselationControl.totalOutputComponentMaxNum
		= m_caps.shaderStage.tesselationControl.patchPointMaxNum * m_caps.shaderStage.tesselationControl.perVertexOutputComponentMaxNum
		+ m_caps.shaderStage.tesselationControl.perPatchOutputComponentMaxNum;

	m_caps.shaderStage.tesselationEvaluation.inputComponentMaxNum = D3D12_DS_INPUT_CONTROL_POINT_REGISTER_COUNT * D3D12_DS_INPUT_CONTROL_POINT_REGISTER_COMPONENTS;
	m_caps.shaderStage.tesselationEvaluation.outputComponentMaxNum = D3D12_DS_INPUT_CONTROL_POINT_REGISTER_COUNT * D3D12_DS_INPUT_CONTROL_POINT_REGISTER_COMPONENTS;

	m_caps.shaderStage.geometry.invocationMaxNum = D3D12_GS_MAX_INSTANCE_COUNT;
	m_caps.shaderStage.geometry.inputComponentMaxNum = D3D12_GS_INPUT_REGISTER_COUNT * D3D12_GS_INPUT_REGISTER_COMPONENTS;
	m_caps.shaderStage.geometry.outputComponentMaxNum = D3D12_GS_OUTPUT_REGISTER_COUNT * D3D12_GS_INPUT_REGISTER_COMPONENTS;
	m_caps.shaderStage.geometry.outputVertexMaxNum = D3D12_GS_MAX_OUTPUT_VERTEX_COUNT_ACROSS_INSTANCES;
	m_caps.shaderStage.geometry.totalOutputComponentMaxNum = D3D12_REQ_GS_INVOCATION_32BIT_OUTPUT_COMPONENT_LIMIT;

	m_caps.shaderStage.fragment.inputComponentMaxNum = D3D12_PS_INPUT_REGISTER_COUNT * D3D12_PS_INPUT_REGISTER_COMPONENTS;
	m_caps.shaderStage.fragment.attachmentMaxNum = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
	m_caps.shaderStage.fragment.dualSourceAttachmentMaxNum = 1;

	m_caps.shaderStage.compute.workGroupMaxNum[0] = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
	m_caps.shaderStage.compute.workGroupMaxNum[1] = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
	m_caps.shaderStage.compute.workGroupMaxNum[2] = D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION;
	m_caps.shaderStage.compute.workGroupMaxDim[0] = D3D12_CS_THREAD_GROUP_MAX_X;
	m_caps.shaderStage.compute.workGroupMaxDim[1] = D3D12_CS_THREAD_GROUP_MAX_Y;
	m_caps.shaderStage.compute.workGroupMaxDim[2] = D3D12_CS_THREAD_GROUP_MAX_Z;
	m_caps.shaderStage.compute.workGroupInvocationMaxNum = D3D12_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
	m_caps.shaderStage.compute.sharedMemoryMaxSize = D3D12_CS_THREAD_LOCAL_TEMP_REGISTER_POOL;

	m_caps.shaderStage.rayTracing.shaderGroupIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	m_caps.shaderStage.rayTracing.tableMaxStride = D3D12_RAYTRACING_MAX_SHADER_RECORD_STRIDE;
	m_caps.shaderStage.rayTracing.recursionMaxDepth = D3D12_RAYTRACING_MAX_DECLARABLE_TRACE_RECURSION_DEPTH;

	m_caps.shaderStage.meshControl.sharedMemoryMaxSize = 32 * 1024;
	m_caps.shaderStage.meshControl.workGroupInvocationMaxNum = 128;
	m_caps.shaderStage.meshControl.payloadMaxSize = 16 * 1024;

	m_caps.shaderStage.meshEvaluation.outputVerticesMaxNum = 256;
	m_caps.shaderStage.meshEvaluation.outputPrimitiveMaxNum = 256;
	m_caps.shaderStage.meshEvaluation.outputComponentMaxNum = 128;
	m_caps.shaderStage.meshEvaluation.sharedMemoryMaxSize = 28 * 1024;
	m_caps.shaderStage.meshEvaluation.workGroupInvocationMaxNum = 128;

	m_caps.wave.laneMinNum = options1.WaveLaneCountMin;
	m_caps.wave.laneMaxNum = options1.WaveLaneCountMax;

	m_caps.wave.derivativeOpsStages = ShaderStage::Pixel;
	if (m_caps.shaderModel >= 66) 
	{
		m_caps.wave.derivativeOpsStages |= ShaderStage::Compute;
#ifdef USE_AGILITY_SDK
		if (options9.DerivativesInMeshAndAmplificationShadersSupported)
		{
			m_caps.wave.derivativeOpsStages |= ShaderStage::Amplification | ShaderStage::Mesh;
		}
#endif
	}

	if (m_caps.shaderModel >= 60 && options1.WaveOps) 
	{
		m_caps.wave.waveOpsStages = ShaderStage::All;
		m_caps.wave.quadOpsStages = ShaderStage::Pixel | ShaderStage::Compute;
	}

	m_caps.other.timestampFrequencyHz = GetD3D12DeviceManager()->GetTimestampFrequency();
#ifdef D3D12_HAS_OPACITY_MICROMAP
	m_caps.other.micromapSubdivisionMaxLevel = D3D12_RAYTRACING_OPACITY_MICROMAP_OC1_MAX_SUBDIVISION_LEVEL;
#endif
	m_caps.other.drawIndirectMaxNum = (1ull << D3D12_REQ_DRAWINDEXED_INDEX_COUNT_2_TO_EXP) - 1;
	m_caps.other.samplerLodBiasMax = D3D12_MIP_LOD_BIAS_MAX;
	m_caps.other.samplerAnisotropyMax = D3D12_DEFAULT_MAX_ANISOTROPY;
	m_caps.other.texelOffsetMin = D3D12_COMMONSHADER_TEXEL_OFFSET_MAX_NEGATIVE;
	m_caps.other.texelOffsetMax = D3D12_COMMONSHADER_TEXEL_OFFSET_MAX_POSITIVE;
	m_caps.other.texelGatherOffsetMin = D3D12_COMMONSHADER_TEXEL_OFFSET_MAX_NEGATIVE;
	m_caps.other.texelGatherOffsetMax = D3D12_COMMONSHADER_TEXEL_OFFSET_MAX_POSITIVE;
	m_caps.other.clipDistanceMaxNum = D3D12_CLIP_OR_CULL_DISTANCE_COUNT;
	m_caps.other.cullDistanceMaxNum = D3D12_CLIP_OR_CULL_DISTANCE_COUNT;
	m_caps.other.combinedClipAndCullDistanceMaxNum = D3D12_CLIP_OR_CULL_DISTANCE_COUNT;
	m_caps.other.viewMaxNum = options3.ViewInstancingTier != D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED ? D3D12_MAX_VIEW_INSTANCE_COUNT : 1;

	m_caps.tiers.conservativeRaster = (uint8_t)options.ConservativeRasterizationTier;
	m_caps.tiers.sampleLocations = (uint8_t)options2.ProgrammableSamplePositionsTier;

	if (options.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_3 && shaderModel.HighestShaderModel >= D3D_SHADER_MODEL_6_6)
	{
		m_caps.tiers.bindless = 2;
	}
	else if (levels.MaxSupportedFeatureLevel >= D3D_FEATURE_LEVEL_12_0)
	{
		m_caps.tiers.bindless = 1;
	}

	if (options.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_3)
	{
		m_caps.tiers.resourceBinding = 2;
	}
	else if (options.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_2)
	{
		m_caps.tiers.resourceBinding = 1;
	}

	m_caps.features.getMemoryDesc2 = true;
	m_caps.features.swapChain = true; // TODO: HasOutput();
	m_caps.features.lowLatency = false; // TODO HasNvExt();
	m_caps.features.micromap = m_caps.tiers.rayTracing >= 3;

	m_caps.features.textureFilterMinMax = levels.MaxSupportedFeatureLevel >= D3D_FEATURE_LEVEL_11_1 ? true : false;
	m_caps.features.logicOp = options.OutputMergerLogicOp != 0;
	m_caps.features.depthBoundsTest = options2.DepthBoundsTestSupported != 0;
	m_caps.features.drawIndirectCount = true;
	m_caps.features.lineSmoothing = true;
	m_caps.features.regionResolve = true;
	m_caps.features.flexibleMultiview = options3.ViewInstancingTier != D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED;
	m_caps.features.layerBasedMultiview = options3.ViewInstancingTier != D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED;
	m_caps.features.viewportBasedMultiview = options3.ViewInstancingTier != D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED;
	m_caps.features.waitableSwapChain = true; // TODO: swap chain version >= 2?
	m_caps.features.pipelineStatistics = true;

	bool isShaderAtomicsF16Supported = false;
	bool isShaderAtomicsF32Supported = false;
	// TODO:
#if NRI_ENABLE_NVAPI
	if (HasNvExt()) {
		REPORT_ERROR_ON_BAD_NVAPI_STATUS(this, NvAPI_D3D12_IsNvShaderExtnOpCodeSupported(m_Device, NV_EXTN_OP_FP16_ATOMIC, &isShaderAtomicsF16Supported));
		REPORT_ERROR_ON_BAD_NVAPI_STATUS(this, NvAPI_D3D12_IsNvShaderExtnOpCodeSupported(m_Device, NV_EXTN_OP_FP32_ATOMIC, &isShaderAtomicsF32Supported));
	}
#endif

	m_caps.shaderFeatures.nativeI16 = options4.Native16BitShaderOpsSupported;
	m_caps.shaderFeatures.nativeF16 = options4.Native16BitShaderOpsSupported;
	m_caps.shaderFeatures.nativeI64 = options1.Int64ShaderOps;
	m_caps.shaderFeatures.nativeF64 = options.DoublePrecisionFloatShaderOps;
	m_caps.shaderFeatures.atomicsF16 = isShaderAtomicsF16Supported;
	m_caps.shaderFeatures.atomicsF32 = isShaderAtomicsF32Supported;
#ifdef USE_AGILITY_SDK
	m_caps.shaderFeatures.atomicsI64 = m_caps.shaderFeatures.atomicsI64 || options9.AtomicInt64OnTypedResourceSupported || options9.AtomicInt64OnGroupSharedSupported || options11.AtomicInt64OnDescriptorHeapResourceSupported;
#endif
	m_caps.shaderFeatures.viewportIndex = options.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation;
	m_caps.shaderFeatures.layerIndex = options.VPAndRTArrayIndexFromAnyShaderFeedingRasterizerSupportedWithoutGSEmulation;
	m_caps.shaderFeatures.rasterizedOrderedView = options.ROVsSupported;
	m_caps.shaderFeatures.barycentric = options3.BarycentricsSupported;
	m_caps.shaderFeatures.storageReadWithoutFormat = true; // All desktop GPUs support it since 2014
	m_caps.shaderFeatures.storageWriteWithoutFormat = true;

	if (m_caps.wave.waveOpsStages != ShaderStage::None) 
	{
		m_caps.shaderFeatures.waveQuery = true;
		m_caps.shaderFeatures.waveVote = true;
		m_caps.shaderFeatures.waveShuffle = true;
		m_caps.shaderFeatures.waveArithmetic = true;
		m_caps.shaderFeatures.waveReduction = true;
		m_caps.shaderFeatures.waveQuad = true;
	}
}


DescriptorHandle2 Device::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
{
	lock_guard lock(m_freeDescriptorMutexes[heapType]);

	DescriptorHandle2 descriptorHandle{};

	// Create a new descriptor heap if there is no a free descriptor
	auto& freeDescriptors = m_freeDescriptors[heapType];
	if (freeDescriptors.empty())
	{
		lock_guard lock2(m_descriptorHeapLock);

		// Can't create a new heap because the index doesn't fit into "DESCRIPTOR_HANDLE_HEAP_INDEX_BIT_NUM" bits
		size_t heapIndex = m_descriptorHeaps.size();
		assert(heapIndex < (1 << DESCRIPTOR_HANDLE_HEAP_INDEX_BIT_NUM));

		// Create a new batch of descriptors
		D3D12_DESCRIPTOR_HEAP_DESC desc{
			.Type = heapType,
			.NumDescriptors = DESCRIPTOR_BATCH_SIZE,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			.NodeMask = 1
		};

		wil::com_ptr<ID3D12DescriptorHeap> descriptorHeap;
		assert_succeeded(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));

		DescriptorHeapDesc descriptorHeapDesc{
			.heap = descriptorHeap,
			.basePointerCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr,
			.descriptorSize = m_device->GetDescriptorHandleIncrementSize(heapType)
		};

		m_descriptorHeaps.push_back(descriptorHeapDesc);

		// Add the new batch of free descriptors
		freeDescriptors.reserve(desc.NumDescriptors);

		for (uint32_t i = 0; i < desc.NumDescriptors; ++i)
		{
			DescriptorHandle2 handle{
				.heapType		= (uint32_t)heapType,
				.heapIndex		= (uint32_t)heapIndex,
				.heapOffset		= i,
				.allocated		= true
			};

			freeDescriptors.push_back(handle);
		}
	}

	// Reserve and return one descriptor
	descriptorHandle = freeDescriptors.back();
	freeDescriptors.pop_back();

	return descriptorHandle;
}


void Device::FreeDescriptorHandle(const DescriptorHandle2& handle)
{
	lock_guard lock(m_freeDescriptorMutexes[handle.heapType]);

	auto& freeDescriptors = m_freeDescriptors[handle.heapType];
	freeDescriptors.push_back(handle);
}


D3D12_CPU_DESCRIPTOR_HANDLE Device::GetDescriptorHandleCPU(const DescriptorHandle2 handle)
{
	lock_guard lock(m_descriptorHeapLock);

	const DescriptorHeapDesc& descriptorHeapDesc = m_descriptorHeaps[handle.heapIndex];
	D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandleCPU{ descriptorHeapDesc.basePointerCPU + handle.heapOffset * descriptorHeapDesc.descriptorSize };

	return descriptorHandleCPU;
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


template <class TPipelineStream>
GraphicsPipelinePtr Device::CreateGraphicsPipelineStream(TPipelineStream& pipelineStream, size_t hashCode, const GraphicsPipelineDesc& pipelineDesc)
{
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

		D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {};
		pipelineStateStreamDesc.pPipelineStateSubobjectStream = &pipelineStream;
		pipelineStateStreamDesc.SizeInBytes = sizeof(pipelineStream);

		HRESULT res = m_device2->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&pPipelineState));
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


Device* GetD3D12Device()
{
	assert(g_d3d12Device != nullptr);

	return g_d3d12Device;
}

} // namespace Luna::DX12