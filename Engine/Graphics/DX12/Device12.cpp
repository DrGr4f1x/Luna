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
#include "DescriptorHeap12.h"
#include "DescriptorSet12.h"
#include "DeviceCaps12.h"
#include "DeviceManager12.h"
#include "GpuBuffer12.h"
#include "PipelineState12.h"
#include "PipelineStateUtil12.h"
#include "QueryHeap12.h"
#include "RootSignature12.h"
#include "Sampler12.h"
#include "Shader12.h"
#include "Texture12.h"

#include "RayTracing\RayTracingGeometry12.h"
#include "RayTracing\RayTracingPipeline12.h"
#include "RayTracing\ShaderBindingTable12.h"

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
	m_dxrDevice = m_device.query<ID3D12Device5>();

	assert(g_d3d12Device == nullptr);
	g_d3d12Device = this;
}


DescriptorHeapPtr Device::CreateDescriptorHeap(uint32_t numImages, uint32_t numBuffers, uint32_t numSamplers)
{
	auto descriptorHeap = make_shared<DescriptorHeap>();

	const uint32_t numResources = numImages + numBuffers;
	descriptorHeap->m_resourceHeap.heap = CreateDescriptorHeapInternal(numResources, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
	descriptorHeap->m_resourceHeap.numDescriptors = numResources;
	
	descriptorHeap->m_samplerHeap.heap = CreateDescriptorHeapInternal(numSamplers, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, true);
	descriptorHeap->m_samplerHeap.numDescriptors = numSamplers;

	descriptorHeap->Initialize();

	return descriptorHeap;
}


Luna::ColorBufferPtr Device::CreateColorBuffer(const ColorBufferDesc& colorBufferDesc)
{
	// Make a writable copy of the ColorBufferDesc, since we need to do
	// a few DirectX-specific calculations and conversions.

	ColorBufferDesc colorBufferDesc2 = colorBufferDesc;
	colorBufferDesc2.planeCount = GetFormatPlaneCount(FormatToDxgi(colorBufferDesc.format).resourceFormat);

	// Compute number of mips
	auto numMips = colorBufferDesc.numMips == 0 ? ComputeNumMips(colorBufferDesc.width, colorBufferDesc.height) : colorBufferDesc.numMips;
	colorBufferDesc2.numMips = numMips;

	// Create resource
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

	// Create Luna DX12 ColorBuffer
	auto colorBuffer = std::make_shared<Luna::DX12::ColorBuffer>(colorBufferDesc2, this, resource.get());

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
	// Make a writable copy of the DepthBufferDesc, since we need to do
	// a few DirectX-specific calculations and conversions.
	DepthBufferDesc depthBufferDesc2 = depthBufferDesc;
	depthBufferDesc2.planeCount = GetFormatPlaneCount(FormatToDxgi(depthBufferDesc.format).resourceFormat);

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

	// Create Luna DX12 DepthBuffer object
	auto depthBuffer = std::make_shared<Luna::DX12::DepthBuffer>(depthBufferDesc2, this, resource.get());

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
	// Make a writable copy of the GpuBufferDesc, since we need to do
	// a few DirectX-specific calculations and conversions.

	GpuBufferDesc gpuBufferDesc2 = gpuBufferDescIn;
	gpuBufferDesc2.usageState = ResourceState::GenericRead;

	if (gpuBufferDescIn.resourceType == ResourceType::ConstantBuffer)
	{
		gpuBufferDesc2.elementSize = Math::AlignUp(gpuBufferDescIn.elementSize, 256);
	}

	// Allocate buffer
	auto allocation = AllocateBuffer(gpuBufferDesc2);
	auto resource = allocation->GetResource();

	// Create Luna DX12 GpuBuffer object
	auto gpuBuffer = std::make_shared<GpuBuffer>(gpuBufferDesc2, this, allocation.get());

	const size_t bufferSize = gpuBuffer->GetBufferSize();

	if (gpuBufferDesc2.resourceType == ResourceType::ByteAddressBuffer || gpuBufferDesc2.resourceType == ResourceType::IndirectArgsBuffer)
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

		gpuBuffer->m_srvDescriptor.CreateShaderResourceView(resource, srvDesc);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{
			.Format				= DXGI_FORMAT_R32_TYPELESS,
			.ViewDimension		= D3D12_UAV_DIMENSION_BUFFER,
			.Buffer	= {
				.NumElements	= (uint32_t)bufferSize / 4,
				.Flags			= D3D12_BUFFER_UAV_FLAG_RAW
			}
		};

		gpuBuffer->m_uavDescriptor.CreateUnorderedAccessView(resource, uavDesc);
	}

	if (gpuBufferDesc2.resourceType == ResourceType::StructuredBuffer)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
			.Format						= DXGI_FORMAT_UNKNOWN,
			.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER,
			.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Buffer = {
				.NumElements			= (uint32_t)gpuBufferDesc2.elementCount,
				.StructureByteStride	= (uint32_t)gpuBufferDesc2.elementSize,
				.Flags					= D3D12_BUFFER_SRV_FLAG_NONE
			}
		};

		gpuBuffer->m_srvDescriptor.CreateShaderResourceView(resource, srvDesc);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{
			.Format				= DXGI_FORMAT_UNKNOWN,
			.ViewDimension		= D3D12_UAV_DIMENSION_BUFFER,
			.Buffer = {
				.NumElements				= (uint32_t)gpuBufferDesc2.elementCount,
				.StructureByteStride		= (uint32_t)gpuBufferDesc2.elementSize,
				.CounterOffsetInBytes		= 0,
				.Flags						= D3D12_BUFFER_UAV_FLAG_NONE
			}
		};

		gpuBuffer->m_uavDescriptor.CreateUnorderedAccessView(resource, uavDesc);
	}

	if (gpuBufferDesc2.resourceType == ResourceType::TypedBuffer)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
			.Format						= FormatToDxgi(gpuBufferDesc2.format).srvFormat,
			.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER,
			.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Buffer = {
				.NumElements	= (uint32_t)gpuBufferDesc2.elementCount,
				.Flags			= D3D12_BUFFER_SRV_FLAG_NONE
			}
		};

		gpuBuffer->m_srvDescriptor.CreateShaderResourceView(resource, srvDesc);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{
			.Format				= FormatToDxgi(gpuBufferDesc2.format).srvFormat,
			.ViewDimension		= D3D12_UAV_DIMENSION_BUFFER,
			.Buffer = {
				.NumElements	= (uint32_t)gpuBufferDesc2.elementCount,
				.Flags			= D3D12_BUFFER_UAV_FLAG_NONE
			}
		};

		gpuBuffer->m_uavDescriptor.CreateUnorderedAccessView(resource, uavDesc);
	}

	if (gpuBufferDesc2.resourceType == ResourceType::ConstantBuffer)
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{
			.BufferLocation		= resource->GetGPUVirtualAddress(),
			.SizeInBytes		= (uint32_t)bufferSize
		};

		gpuBuffer->m_cbvDescriptor.CreateConstantBufferView(cbvDesc);
	}

	if (gpuBufferDesc2.resourceType == ResourceType::VertexBuffer || gpuBufferDesc2.resourceType == ResourceType::IndexBuffer)
	{
		if (gpuBufferDesc2.bAllowShaderResource)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
				.Format						= FormatToDxgi(gpuBufferDesc2.format).resourceFormat,
				.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER,
				.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
				.Buffer = {
					.NumElements			= (uint32_t)gpuBufferDesc2.elementCount,
					.StructureByteStride	= (uint32_t)gpuBufferDesc2.elementSize,
					.Flags					= D3D12_BUFFER_SRV_FLAG_NONE
				}
			};

			gpuBuffer->m_srvDescriptor.CreateShaderResourceView(resource, srvDesc);
		}

		if (gpuBufferDesc2.bAllowUnorderedAccess)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{
				.Format			= FormatToDxgi(gpuBufferDesc2.format).resourceFormat,
				.ViewDimension	= D3D12_UAV_DIMENSION_BUFFER,
				.Buffer = {
					.NumElements			= (uint32_t)gpuBufferDesc2.elementCount,
					.StructureByteStride	= (uint32_t)gpuBufferDesc2.elementSize,
					.CounterOffsetInBytes	= 0,
					.Flags					= D3D12_BUFFER_UAV_FLAG_NONE
				}
			};

			gpuBuffer->m_uavDescriptor.CreateUnorderedAccessView(resource, uavDesc);
		}
	}

	if (gpuBufferDescIn.initialData)
	{
		if (gpuBuffer->m_type == ResourceType::ConstantBuffer)
		{
			const size_t initialSize = gpuBuffer->GetBufferSize();
			gpuBuffer->Update(initialSize, gpuBufferDescIn.initialData);
		}
		else
		{
			GpuBufferPtr temp = gpuBuffer;
			CommandContext::InitializeBuffer(temp, gpuBufferDescIn.initialData, gpuBuffer->GetBufferSize());
		}
	}

	return gpuBuffer;
}


Luna::RootSignaturePtr Device::CreateRootSignature(DescriptorHeapPtr descriptorHeap, const RootSignatureDesc& rootSignatureDesc)
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
				if (range.descriptorType != DescriptorType::Sampler)
				{
					d3d12Range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
				}
				else
				{
					d3d12Range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE;
				}
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

	// Setup flags
	auto flags = ShaderStageToRootSignatureFlags(combinedShaderStages);
	if (rootSignatureDesc.allowStreamOut)
	{
		flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT;
	}

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC d3d12RootSignatureDesc{
		.Version			= D3D_ROOT_SIGNATURE_VERSION_1_1,
		.Desc_1_1 = {
			.NumParameters			= (uint32_t)d3d12RootParameters.size(),
			.pParameters			= d3d12RootParameters.data(),
			.NumStaticSamplers		= (uint32_t)staticSamplers.size(),
			.pStaticSamplers		= staticSamplers.data(),
			.Flags					= flags
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
	rootSignature->SetDescriptorHeap(descriptorHeap);

	uint32_t rootIndex = 0;
	for (const auto rootParameter : rootSignatureDesc.rootParameters)
	{
		if (rootParameter.parameterType == RootParameterType::Table)
		{
			rootSignature->m_descriptorOffsetTables[rootIndex] = make_shared<DescriptorOffsetTable>(rootParameter);
		}
		++rootIndex;
	}

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


MeshletPipelinePtr Device::CreateMeshletPipeline(const MeshletPipelineDesc& pipelineDesc)
{
	MeshletPipelineContext context{};
	FillMeshletPipelineDesc(context, pipelineDesc);

#if defined(D3D12_SDK_VERSION) && (D3D12_SDK_VERSION >= 610)
	if (m_caps.features.rasterizerDesc2)
	{
		CD3DX12_PIPELINE_STATE_STREAM5 pipelineStream5{};
		FillMeshletPipelineStateStream5(pipelineStream5, context.stateDesc, pipelineDesc);
		return CreateMeshletPipelineStream<CD3DX12_PIPELINE_STATE_STREAM5>(pipelineStream5, context.hashCode, pipelineDesc);
	}
	else
	{
		CD3DX12_PIPELINE_STATE_STREAM4 pipelineStream4{};
		FillMeshletPipelineStateStream4(pipelineStream4, context.stateDesc, pipelineDesc);
		return CreateMeshletPipelineStream<CD3DX12_PIPELINE_STATE_STREAM4>(pipelineStream4, context.hashCode, pipelineDesc);
	}
#endif

#if defined(D3D12_SDK_VERSION) && (D3D12_SDK_VERSION >= 608)
	{
		CD3DX12_PIPELINE_STATE_STREAM4 pipelineStream4{};
		FillMeshletPipelineStateStream4(pipelineStream4, context.stateDesc, pipelineDesc);
		return CreateMeshletPipelineStream<CD3DX12_PIPELINE_STATE_STREAM4>(pipelineStream4, context.hashCode, pipelineDesc);
	}
#endif

#if defined(D3D12_SDK_VERSION) && (D3D12_SDK_VERSION >= 606)
	{
		CD3DX12_PIPELINE_STATE_STREAM3 pipelineStream3{};
		FillMeshletPipelineStateStream3(pipelineStream3, context.stateDesc, pipelineDesc);
		return CreateMeshletPipelineStream<CD3DX12_PIPELINE_STATE_STREAM3>(pipelineStream3, context.hashCode, pipelineDesc);
	}
#endif


	CD3DX12_PIPELINE_STATE_STREAM2 pipelineStream2{};
	FillMeshletPipelineStateStream2(pipelineStream2, context.stateDesc, pipelineDesc);
	return CreateMeshletPipelineStream<CD3DX12_PIPELINE_STATE_STREAM2>(pipelineStream2, context.hashCode, pipelineDesc);
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


GpuBufferPtr Device::CreateBLAS(const RayTracingGeometryDesc& geometryDesc)
{
	D3D12_RAYTRACING_GEOMETRY_DESC d3d12GeometryDesc{
		.Type	= RayTracingGeometryTypeToDX12(geometryDesc.geometryType),
		.Flags	= RayTracingGeometryFlagsToDX12(geometryDesc.flags)
	};

	if (geometryDesc.geometryType == RayTracingGeometryType::Triangles)
	{
		d3d12GeometryDesc.Triangles.IndexBuffer = geometryDesc.triangles.indexBuffer;
		d3d12GeometryDesc.Triangles.IndexCount = (UINT)geometryDesc.triangles.indexCount;
		d3d12GeometryDesc.Triangles.IndexFormat = geometryDesc.triangles.indexFormat == Format::R32_UInt ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
		d3d12GeometryDesc.Triangles.Transform3x4 = geometryDesc.triangles.transform3x4;
		d3d12GeometryDesc.Triangles.VertexFormat = FormatToDxgi(geometryDesc.triangles.vertexFormat).srvFormat;
		d3d12GeometryDesc.Triangles.VertexCount = (UINT)geometryDesc.triangles.vertexCount;
		d3d12GeometryDesc.Triangles.VertexBuffer.StartAddress = geometryDesc.triangles.vertexBuffer;
		d3d12GeometryDesc.Triangles.VertexBuffer.StrideInBytes = geometryDesc.triangles.vertexStride;
	}
	else if (geometryDesc.geometryType == RayTracingGeometryType::AABBs)
	{
		// TODO
		assert(false);
	}
	else
	{
		// TODO
		assert(false);
	}

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	// Get size info
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo{};
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs{
		.Type				= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
		.Flags				= buildFlags,
		.NumDescs			= 1,
		.DescsLayout		= D3D12_ELEMENTS_LAYOUT_ARRAY,
		.pGeometryDescs		= &d3d12GeometryDesc
	};
	m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);

	// Create scratch buffer and BLAS buffer
	GpuBufferPtr scratchBuffer = CreateRayTracingBuffer(bottomLevelPrebuildInfo.ScratchDataSizeInBytes, "Scratch Buffer", D3D12_RESOURCE_STATE_COMMON);
	GpuBufferPtr blasBuffer = CreateRayTracingBuffer(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, "BLAS Buffer", D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

	// Build the BLAS
	RayTracingBuildInfo buildInfo{};
	buildInfo.scratchBuffer = scratchBuffer;
	buildInfo.destBuffer = blasBuffer;
	buildInfo.flags = RayTracingAccelerationStructureFlags::FastTrace;
	buildInfo.type = RayTracingAccelerationStructureType::BottomLevel;
	buildInfo.inputs = bottomLevelInputs;

	CommandContext::BuildAccelerationStructure(&buildInfo);

	return blasBuffer;
}


GpuBufferPtr Device::CreateTLAS(const RayTracingTLASDesc& tlasDesc)
{
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	// Get size info
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo{};
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs{
		.Type			= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
		.Flags			= buildFlags,
		.NumDescs		= 1,
		.DescsLayout	= D3D12_ELEMENTS_LAYOUT_ARRAY
	};
	m_dxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);

	// Create scratch buffer and TLAS buffer
	GpuBufferPtr scratchBuffer = CreateRayTracingBuffer(topLevelPrebuildInfo.ScratchDataSizeInBytes, "Scratch Buffer", D3D12_RESOURCE_STATE_COMMON);
	GpuBufferPtr tlasBuffer = CreateRayTracingBuffer(topLevelPrebuildInfo.ResultDataMaxSizeInBytes, "TLAS Buffer", D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

	// Upload instance data
	vector<D3D12_RAYTRACING_INSTANCE_DESC> d3d12Instances;
	for (const auto& instance : tlasDesc.instances)
	{
		D3D12_RAYTRACING_INSTANCE_DESC instanceDesc{};
		instanceDesc.Transform[0][0] = instance.transform.GetX().GetX();
		instanceDesc.Transform[0][1] = instance.transform.GetX().GetY();
		instanceDesc.Transform[0][2] = instance.transform.GetX().GetZ();
		instanceDesc.Transform[0][3] = instance.transform.GetX().GetW();

		instanceDesc.Transform[1][0] = instance.transform.GetY().GetX();
		instanceDesc.Transform[1][1] = instance.transform.GetY().GetY();
		instanceDesc.Transform[1][2] = instance.transform.GetY().GetZ();
		instanceDesc.Transform[1][3] = instance.transform.GetY().GetW();

		instanceDesc.Transform[2][0] = instance.transform.GetZ().GetX();
		instanceDesc.Transform[2][1] = instance.transform.GetZ().GetY();
		instanceDesc.Transform[2][2] = instance.transform.GetZ().GetZ();
		instanceDesc.Transform[2][3] = instance.transform.GetZ().GetW();

		instanceDesc.InstanceID = instance.instanceID;
		instanceDesc.InstanceMask = instance.instanceMask;
		instanceDesc.AccelerationStructure = instance.blas->GetGpuAddress();

		d3d12Instances.push_back(instanceDesc);
	}

	auto instanceBuffer = CreateRayTracingInstanceBuffer(d3d12Instances.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC), d3d12Instances.data(), "Instance Buffer");

	topLevelInputs.InstanceDescs = instanceBuffer->GetGpuAddress();

	// Build the BLAS
	RayTracingBuildInfo buildInfo{};
	buildInfo.scratchBuffer = scratchBuffer;
	buildInfo.destBuffer = tlasBuffer;
	buildInfo.flags = RayTracingAccelerationStructureFlags::FastTrace;
	buildInfo.type = RayTracingAccelerationStructureType::BottomLevel;
	buildInfo.inputs = topLevelInputs;

	CommandContext::BuildAccelerationStructure(&buildInfo);

	return tlasBuffer;
}


RayTracingPipelinePtr Device::CreateRayTracingPipeline(const RayTracingPipelineDesc& pipelineDesc)
{
	RootSignature* rootSignature12 = (RootSignature*)pipelineDesc.rootSignature.get();
	ID3D12RootSignature* d3d12RootSignature = rootSignature12->GetRootSignature();

	CD3DX12_STATE_OBJECT_DESC raytracingPipelineDesc{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

	// Pipeline config
	if (m_caps.tiers.rayTracing > 1)
	{
		auto pipelineConfig = raytracingPipelineDesc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG1_SUBOBJECT>();
		pipelineConfig->Config(
			pipelineDesc.maxRecursionDepth,
			RayTracingPipelineFlagsToDX12(pipelineDesc.flags)
		);
	}
	else
	{
		auto pipelineConfig = raytracingPipelineDesc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
		pipelineConfig->Config(pipelineDesc.maxRecursionDepth);
	}
	
	// Shader config
	auto shaderConfig = raytracingPipelineDesc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
	shaderConfig->Config(pipelineDesc.maxPayloadSize, pipelineDesc.maxRayHitAttributeSize);

	// Node mask
	auto nodeMaskConfig = raytracingPipelineDesc.CreateSubobject<CD3DX12_NODE_MASK_SUBOBJECT>();
	nodeMaskConfig->SetNodeMask(0x1); // No mGPU support

	// Global root signature
	auto globalRootSignature = raytracingPipelineDesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
	globalRootSignature->SetRootSignature(d3d12RootSignature);

	// Shader and hit-group names, for building the SBT later
	wstring rayGenShaderName;
	wstring missShaderName;
	vector<wstring> hitGroupNames;

	// DXIL library
	std::vector<Shader*> shaders;
	for (const auto& libraryEntry : pipelineDesc.shaderLibrary)
	{
		auto dxilLibrary = raytracingPipelineDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
		const auto shaderType = libraryEntry.type;
		const auto& shaderNameAndEntry = libraryEntry.shaderNameAndEntry;

		Shader* shader = LoadShader(shaderType, shaderNameAndEntry);
		shaders.push_back(shader);

		if (shader->GetShaderType() == ShaderType::RayGeneration)
		{
			assert(rayGenShaderName.empty());
			rayGenShaderName = shader->GetEntryW();
		}
		else if (shader->GetShaderType() == ShaderType::Miss)
		{
			assert(missShaderName.empty());
			missShaderName = shader->GetEntryW();
		}

		D3D12_SHADER_BYTECODE libdxil = CD3DX12_SHADER_BYTECODE(shader->GetByteCode(), shader->GetByteCodeSize());
		dxilLibrary->SetDXILLibrary(&libdxil);
		dxilLibrary->DefineExport(shader->GetEntryW().c_str());
	}

	// Hit groups
	for (uint32_t i = 0; i < (uint32_t)pipelineDesc.shaderGroups.size(); ++i)
	{
		// Loop over the indices in the shader group
		for (uint32_t j = 0; j < 3; ++j)
		{
			const int32_t lookupIndex = pipelineDesc.shaderGroups[i].shaderIndices[j];
			if (lookupIndex == -1)
				continue;

			Shader* shader = shaders[lookupIndex];

			const auto shaderType = shader->GetShaderType();

			// Not a hit group
			if (shaderType == ShaderType::RayGeneration || shaderType == ShaderType::Miss || shaderType == ShaderType::Callable)
			{
				break;
			}

			auto hitgroup = raytracingPipelineDesc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
			hitgroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

			switch (shaderType)
			{
			case ShaderType::Intersection:
				hitgroup->SetIntersectionShaderImport(shader->GetEntryW().c_str());
				hitgroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE);
				break;

			case ShaderType::ClosestHit:
				hitgroup->SetClosestHitShaderImport(shader->GetEntryW().c_str());
				break;

			case ShaderType::AnyHit:
				hitgroup->SetAnyHitShaderImport(shader->GetEntryW().c_str());
				break;
			}

			std::wstring hitGroupName = std::format(L"hitgroup_{}", i);
			hitgroup->SetHitGroupExport(hitGroupName.c_str());
			hitGroupNames.push_back(hitGroupName);

		} // for loop over shaders in shader group

	} // for loop over shader groups

	// Now we can create the actual DX12 object for the raytracing pipeline
	wil::com_ptr<ID3D12StateObject> stateObject;
	m_dxrDevice->CreateStateObject(raytracingPipelineDesc, IID_PPV_ARGS(&stateObject));

	wil::com_ptr<ID3D12StateObjectProperties> stateObjectProperties = stateObject.query<ID3D12StateObjectProperties>();
	
	// Create and return the Luna RayTracingPipeline object
	auto raytracingPipeline = std::make_shared<RayTracingPipeline>();
	raytracingPipeline->m_stateObject = stateObject;
	raytracingPipeline->m_stateObjectProperties = stateObjectProperties;
	raytracingPipeline->m_rayGenShaderName = rayGenShaderName;
	raytracingPipeline->m_missShaderName = missShaderName;
	raytracingPipeline->m_hitGroupNames = std::move(hitGroupNames);
	return raytracingPipeline;
}


ShaderBindingTablePtr Device::CreateShaderBindingTable(RayTracingPipelinePtr pipeline)
{
	// Get the SBT alignment and the size of each SBT record
	const size_t sbtRecordSize = m_caps.shaderStage.rayTracing.shaderGroupIdentifierSize;
	const size_t tableAlignment = m_caps.memoryAlignment.shaderBindingTable;
	const uint32_t sbtRecordStride = (uint32_t)Math::AlignUp(sbtRecordSize, tableAlignment);

	// Compute table size
	const size_t numSbtRecords =
		(pipeline->HasRayGenShader() ? 1 : 0) +
		(pipeline->HasMissShader() ? 1 : 0) +
		pipeline->GetNumHitGroups();
	assert(numSbtRecords > 0);
	const size_t sbtSize = sbtRecordStride * numSbtRecords;

	// Describe buffer
	GpuBufferDesc sbtDesc{
		.name					= "Shader Binding Table",
		.resourceType			= ResourceType::UploadBuffer,
		.memoryAccess			= MemoryAccess::CpuWrite | MemoryAccess::GpuRead,
		.elementCount			= 1,
		.elementSize			= sbtSize,
		.bShaderBindingTable	= true
	};

	// Allocate buffer
	auto allocation = AllocateBuffer(sbtDesc);

	// Create Luna DX12 GpuBuffer object
	auto gpuBuffer = std::make_shared<GpuBuffer>(sbtDesc, this, allocation.get());

	// Map buffer memory for SBT record uploads
	uint64_t gpuAddress = gpuBuffer->GetGpuAddress();
	std::byte* mem = (std::byte*)gpuBuffer->Map();

	// Copy SBT records
	RayTracingPipeline* pipeline12 = (RayTracingPipeline*)pipeline.get();
	ID3D12StateObjectProperties* properties = pipeline12->GetStateObjectProperties();

	StridedAddressRegion rayGenRegion{};
	StridedAddressRegion missRegion{};
	StridedAddressRegion hitGroupRegion{};

	// Ray gen shader record
	if (pipeline->HasRayGenShader())
	{
		void* shaderIdentifier = properties->GetShaderIdentifier(pipeline12->GetRayGenerationShaderName());
		memcpy(mem, shaderIdentifier, sbtRecordSize);
		mem += sbtRecordStride;

		rayGenRegion.bufferAddress = gpuAddress;
		rayGenRegion.size = sbtRecordStride;
		rayGenRegion.stride = sbtRecordStride;
		gpuAddress += sbtRecordStride;
	}

	// Miss shader record
	if (pipeline->HasMissShader())
	{
		void* shaderIdentifier = properties->GetShaderIdentifier(pipeline12->GetMissShaderName());
		memcpy(mem, shaderIdentifier, sbtRecordSize);
		mem += sbtRecordStride;

		missRegion.bufferAddress = gpuAddress;
		missRegion.size = sbtRecordStride;
		missRegion.stride = sbtRecordStride;
		gpuAddress += sbtRecordStride;
	}

	// Hit group shader records
	for (uint32_t i = 0; i < pipeline->GetNumHitGroups(); ++i)
	{
		void* shaderIdentifier = properties->GetShaderIdentifier(pipeline12->GetHitGroupName(i));
		memcpy(mem, shaderIdentifier, sbtRecordSize);
		mem += sbtRecordStride;
	}
	if (pipeline->GetNumHitGroups() > 0)
	{
		hitGroupRegion.bufferAddress = gpuAddress;
		hitGroupRegion.size = sbtRecordStride * pipeline->GetNumHitGroups();
		hitGroupRegion.stride = sbtRecordStride;
	}

	// Unmap buffer memory
	gpuBuffer->Unmap();

	// Create and return ShaderBindingTable
	auto shaderBindingTable = make_shared<ShaderBindingTable>();
	shaderBindingTable->m_sbt = gpuBuffer;
	shaderBindingTable->m_rayGenRegion = rayGenRegion;
	shaderBindingTable->m_missRegion = missRegion;
	shaderBindingTable->m_hitGroupRegion = hitGroupRegion;

	return shaderBindingTable;
}


ITexture* Device::CreateUninitializedTexture(const std::string& name, const std::string& mapKey)
{
	TextureDesc desc{};
	Texture* tex = new Texture(desc, this, nullptr, name);
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

	D3D12_RESOURCE_DESC resourceDesc = displayPlane->GetDesc();
	const uint8_t planeCount = GetFormatPlaneCount(resourceDesc.Format);

	// Create Luna DX12 ColorBuffer object
	ColorBufferDesc desc{
		.name				= std::format("Primary SwapChain Image {}", imageIndex),
		.width				= resourceDesc.Width,
		.height				= resourceDesc.Height,
		.arraySizeOrDepth	= resourceDesc.DepthOrArraySize,
		.numMips			= 1,
		.numSamples			= resourceDesc.SampleDesc.Count,
		.planeCount			= planeCount,
		.clearColor			= DirectX::Colors::Black
	};

	auto colorBuffer = std::make_shared<Luna::DX12::ColorBuffer>(desc, this, displayPlane.get());

	colorBuffer->m_srvDescriptor.CreateShaderResourceView(displayPlane.get());
	colorBuffer->m_rtvDescriptor.CreateRenderTargetView(displayPlane.get());
	
	return colorBuffer;
}


void Device::FillCaps(const AdapterInfo& adapterInfo)
{
	HRESULT hr{};

	m_caps.adapterInfo = adapterInfo;
	m_caps.api = GraphicsApi::D3D12;

	Luna::DX12::FillCaps(m_device.get(), m_caps);
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


wil::com_ptr<ID3D12DescriptorHeap> Device::CreateDescriptorHeapInternal(uint32_t numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type, bool shaderVisisble)
{
	auto desc = D3D12_DESCRIPTOR_HEAP_DESC{
		.Type				= type,
		.NumDescriptors		= numDescriptors,
		.Flags				= shaderVisisble ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		.NodeMask			= 1
	};

	wil::com_ptr<ID3D12DescriptorHeap> heap;
	assert_succeeded(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));

	SetDebugName(heap.get(), format("DescriptorAllocator [{}]", type));

	return heap;
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
		.Format				= DXGI_FORMAT_UNKNOWN,
		.SampleDesc			= { .Count = 1, .Quality = 0 },
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


template <class TPipelineStream>
MeshletPipelinePtr Device::CreateMeshletPipelineStream(TPipelineStream& pipelineStream, size_t hashCode, const MeshletPipelineDesc& pipelineDesc)
{
	// Find or create meshlet pipeline state object
	ID3D12PipelineState** pipelineStateRef = nullptr;
	bool firstCompile = false;
	{
		lock_guard<mutex> lock(m_meshletPipelineStateMutex);

		auto iter = m_meshletPipelineStateHashMap.find(hashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == m_meshletPipelineStateHashMap.end())
		{
			pipelineStateRef = m_meshletPipelineStateHashMap[hashCode].addressof();
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

		m_meshletPipelineStateHashMap[hashCode].attach(pPipelineState);
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

	auto meshletPipeline = std::make_shared<MeshletPipeline>();

	meshletPipeline->m_device = this;
	meshletPipeline->m_desc = pipelineDesc;
	meshletPipeline->m_pipelineState = pPipelineState;

	return meshletPipeline;
}


GpuBufferPtr Device::CreateRayTracingBuffer(size_t sizeInBytes, const std::string& name, D3D12_RESOURCE_STATES initialState)
{
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	auto allocationDesc = D3D12MA::ALLOCATION_DESC{ .HeapType = D3D12_HEAP_TYPE_DEFAULT };

	wil::com_ptr<D3D12MA::Allocation> allocation;
	HRESULT hr = m_allocator->CreateResource(
		&allocationDesc,
		&resourceDesc,
		initialState,
		NULL,
		&allocation,
		IID_NULL, NULL);

	// Create Luna DX12 GpuBuffer
	GpuBufferDesc desc{
		.name			= name,
		.resourceType	= ResourceType::AccelerationStructure,
		.usageState		= ResourceState::AccelStructBuildBlas,
		.elementSize	= sizeInBytes
	};

	auto gpuBuffer = std::make_shared<GpuBuffer>(desc, this, allocation.get());

	// Setup descriptor for acceleration structure
	gpuBuffer->m_srvDescriptor.CreateAccelerationStructureView(allocation->GetResource());

	return gpuBuffer;
}


GpuBufferPtr Device::CreateRayTracingInstanceBuffer(size_t sizeInBytes, const void* initialData, const std::string& name)
{
	auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes, D3D12_RESOURCE_FLAG_NONE, D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT);

	auto allocationDesc = D3D12MA::ALLOCATION_DESC{ .HeapType = D3D12_HEAP_TYPE_UPLOAD };

	wil::com_ptr<D3D12MA::Allocation> allocation;
	HRESULT hr = m_allocator->CreateResource(
		&allocationDesc,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		NULL,
		&allocation,
		IID_NULL, NULL);

	// Create Luna DX12 GpuBuffer
	GpuBufferDesc desc{
		.name			= name,
		.resourceType	= ResourceType::UploadBuffer,
		.usageState		= ResourceState::GenericRead,
		.elementSize	= sizeInBytes
	};

	// Create GpuBuffer
	auto gpuBuffer = std::make_shared<GpuBuffer>(desc, this, allocation.get());

	// Copy instance data into buffer
	void* ptr = gpuBuffer->Map();
	memcpy(ptr, initialData, sizeInBytes);
	gpuBuffer->Unmap();

	return gpuBuffer;
}


Device* GetD3D12Device()
{
	assert(g_d3d12Device != nullptr);

	return g_d3d12Device;
}

} // namespace Luna::DX12