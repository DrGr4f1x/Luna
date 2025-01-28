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

#include "BinaryReader.h"
#include "FileSystem.h"

#include "ColorBuffer12.h"
#include "DepthBuffer12.h"
#include "DescriptorAllocator12.h"
#include "DeviceCaps12.h"
#include "Formats12.h"
#include "GpuBuffer12.h"
#include "PipelineState12.h"
#include "Queue12.h"
#include "RootSignature12.h"
#include "Shader12.h"

using namespace std;
using namespace Microsoft::WRL;


extern Luna::IGraphicsDevice* g_graphicsDevice;

static constexpr uint32_t c_MaxVolatileConstantBuffersPerLayout = 6;


namespace Luna::DX12
{

GraphicsDevice* g_d3d12GraphicsDevice{ nullptr };


void DebugMessageCallback(
	D3D12_MESSAGE_CATEGORY category,
	D3D12_MESSAGE_SEVERITY severity,
	D3D12_MESSAGE_ID id,
	LPCSTR pDescription,
	void* pContext)
{
	string debugMessage = format("[{}] Code {} : {}", category, (uint32_t)id, pDescription);

	switch (severity)
	{
	case D3D12_MESSAGE_SEVERITY_CORRUPTION:
		LogFatal(LogDirectX) << debugMessage << endl;
		break;
	case D3D12_MESSAGE_SEVERITY_ERROR:
		LogError(LogDirectX) << debugMessage << endl;
		break;
	case D3D12_MESSAGE_SEVERITY_WARNING:
		LogWarning(LogDirectX) << debugMessage << endl;
		break;
	case D3D12_MESSAGE_SEVERITY_INFO:
	case D3D12_MESSAGE_SEVERITY_MESSAGE:
		LogInfo(LogDirectX) << debugMessage << endl;
		break;
	}
}


D3D12_RESOURCE_FLAGS CombineResourceFlags(uint32_t fragmentCount)
{
	D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

	if (flags == D3D12_RESOURCE_FLAG_NONE && fragmentCount == 1)
	{
		flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | flags;
}


DescriptorType GetNormalizedDescriptorType(DescriptorType descriptorType)
{
	using enum DescriptorType;

	switch (descriptorType)
	{
	case StructuredBufferUAV:
	case RawBufferUAV:
		return TypedBufferUAV;
		break;

	case StructuredBufferSRV:
	case RawBufferSRV:
		return TypedBufferSRV;

	default:
		return descriptorType;
		break;
	}
}


bool AreDescriptorTypesCompatible(DescriptorType a, DescriptorType b)
{
	using enum DescriptorType;

	if (a == b)
	{
		return true;
	}

	a = GetNormalizedDescriptorType(a);
	b = GetNormalizedDescriptorType(b);

	if ((a == TypedBufferSRV && b == TextureSRV) ||
		(b == TypedBufferSRV && a == TextureSRV) ||
		(a == TypedBufferSRV && b == RayTracingAccelStruct) ||
		(a == TextureSRV && b == RayTracingAccelStruct) ||
		(b == TypedBufferSRV && a == RayTracingAccelStruct) ||
		(b == TextureSRV && a == RayTracingAccelStruct))
	{
		return true;
	}

	if ((a == TypedBufferUAV && b == TextureUAV) ||
		(b == TypedBufferUAV && a == TextureUAV))
	{
		return true;
	}

	return false;
}


inline ID3D12RootSignature* GetRootSignature(const RootSignature& rootSig)
{
	const auto platformData = rootSig.GetPlatformData();
	wil::com_ptr<IRootSignatureData> rootSignatureData;
	assert_succeeded(platformData->QueryInterface(IID_PPV_ARGS(&rootSignatureData)));
	return rootSignatureData->GetRootSignature();
}


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


DeviceRLDOHelper::~DeviceRLDOHelper()
{
	if (device && doReport)
	{
		wil::com_ptr<ID3D12DebugDevice> debugInterface;
		if (SUCCEEDED(device->QueryInterface(debugInterface.addressof())))
		{
			debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
		}
	}
}


GraphicsDevice::GraphicsDevice(const GraphicsDeviceDesc& desc) noexcept
	: m_desc{ desc }
	, m_deviceRLDOHelper{ desc.dx12Device, desc.enableValidation }
{
	LogInfo(LogDirectX) << "Creating DirectX 12 device." << endl;

	m_dxgiFactory = m_desc.dxgiFactory;
	m_dxDevice = m_desc.dx12Device;
	m_d3d12maAllocator = m_desc.d3d12maAllocator;

	SetDebugName(m_dxDevice.get(), "DX12 Device");

	g_graphicsDevice = this;
	g_d3d12GraphicsDevice = this;
}


GraphicsDevice::~GraphicsDevice()
{
	LogInfo(LogDirectX) << "Destroying DirectX 12 device." << endl;

	if (m_dxInfoQueue)
	{
		m_dxInfoQueue->UnregisterMessageCallback(m_callbackCookie);
		m_dxInfoQueue.reset();
	}

	g_d3d12GraphicsDevice = nullptr;
	g_graphicsDevice = nullptr;
}


wil::com_ptr<IPlatformData> GraphicsDevice::CreateColorBufferData(ColorBufferDesc& desc, ResourceState& initialState)
{
	// Create resource
	auto numMips = desc.numMips == 0 ? ComputeNumMips(desc.width, desc.height) : desc.numMips;
	auto flags = CombineResourceFlags(desc.numSamples);

	D3D12_RESOURCE_DESC resourceDesc{
		.Dimension = GetResourceDimension(desc.resourceType),
		.Alignment = 0,
		.Width = (UINT64)desc.width,
		.Height = (UINT)desc.height,
		.DepthOrArraySize = (UINT16)desc.arraySizeOrDepth,
		.MipLevels = (UINT16)numMips,
		.Format = FormatToDxgi(desc.format).resourceFormat,
		.SampleDesc = {.Count = desc.numSamples, .Quality = 0 },
		.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
		.Flags = (D3D12_RESOURCE_FLAGS)flags
	};

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = FormatToDxgi(desc.format).rtvFormat;
	clearValue.Color[0] = desc.clearColor.R();
	clearValue.Color[1] = desc.clearColor.G();
	clearValue.Color[2] = desc.clearColor.B();
	clearValue.Color[3] = desc.clearColor.A();

	ID3D12Resource* resource{ nullptr };
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	assert_succeeded(m_dxDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&resourceDesc, D3D12_RESOURCE_STATE_COMMON, &clearValue, IID_PPV_ARGS(&resource)));

	SetDebugName(resource, desc.name);

	// Create descriptors and derived views
	assert_msg(desc.arraySizeOrDepth == 1 || numMips == 1, "We don't support auto-mips on texture arrays");

	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};

	DXGI_FORMAT dxgiFormat = FormatToDxgi(desc.format).resourceFormat;
	rtvDesc.Format = dxgiFormat;
	uavDesc.Format = GetUAVFormat(dxgiFormat);
	srvDesc.Format = dxgiFormat;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	if (desc.arraySizeOrDepth > 1)
	{
		if (desc.resourceType == ResourceType::Texture3D)
		{
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
			rtvDesc.Texture3D.MipSlice = 0;
			rtvDesc.Texture3D.FirstWSlice = 0;
			rtvDesc.Texture3D.WSize = (UINT)desc.arraySizeOrDepth;

			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
			uavDesc.Texture3D.MipSlice = 0;
			uavDesc.Texture3D.FirstWSlice = 0;
			uavDesc.Texture3D.WSize = (UINT)desc.arraySizeOrDepth;

			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			srvDesc.Texture3D.MipLevels = numMips;
			srvDesc.Texture3D.MostDetailedMip = 0;
		}
		else
		{
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			rtvDesc.Texture2DArray.MipSlice = 0;
			rtvDesc.Texture2DArray.FirstArraySlice = 0;
			rtvDesc.Texture2DArray.ArraySize = (UINT)desc.arraySizeOrDepth;

			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			uavDesc.Texture2DArray.MipSlice = 0;
			uavDesc.Texture2DArray.FirstArraySlice = 0;
			uavDesc.Texture2DArray.ArraySize = (UINT)desc.arraySizeOrDepth;

			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			srvDesc.Texture2DArray.MipLevels = numMips;
			srvDesc.Texture2DArray.MostDetailedMip = 0;
			srvDesc.Texture2DArray.FirstArraySlice = 0;
			srvDesc.Texture2DArray.ArraySize = (UINT)desc.arraySizeOrDepth;
		}
	}
	else if (desc.numFragments > 1)
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
	m_dxDevice->CreateRenderTargetView(resource, &rtvDesc, rtvHandle);

	// Create the shader resource view
	m_dxDevice->CreateShaderResourceView(resource, &srvDesc, srvHandle);

	// Create the UAVs for each mip level (RWTexture2D)
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 12> uavHandles;
	if (desc.numFragments == 1)
	{
		for (uint32_t i = 0; i < (uint32_t)uavHandles.size(); ++i)
		{
			uavHandles[i].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		}

		for (uint32_t i = 0; i < numMips; ++i)
		{
			uavHandles[i] = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			m_dxDevice->CreateUnorderedAccessView(resource, nullptr, &uavDesc, uavHandles[i]);

			uavDesc.Texture2D.MipSlice++;
		}
	}

	const uint8_t planeCount = GetFormatPlaneCount(FormatToDxgi(desc.format).resourceFormat);
	desc.planeCount = planeCount;
	initialState = ResourceState::Common;

	auto descExt = ColorBufferDescExt{}
		.SetResource(resource)
		.SetUsageState(ResourceState::Common)
		.SetPlaneCount(planeCount)
		.SetRtvHandle(rtvHandle)
		.SetSrvHandle(srvHandle)
		.SetUavHandles(uavHandles);

	auto colorBufferData = Make<ColorBufferData>(descExt);
	return colorBufferData;
}

wil::com_ptr<IPlatformData> GraphicsDevice::CreateDepthBufferData(DepthBufferDesc& desc, ResourceState& initialState)
{
	// Create resource
	D3D12_RESOURCE_DESC resourceDesc{
		.Dimension			= GetResourceDimension(desc.resourceType),
		.Alignment			= 0,
		.Width				= (UINT64)desc.width,
		.Height				= (UINT)desc.height,
		.DepthOrArraySize	= (UINT16)desc.arraySizeOrDepth,
		.MipLevels			= (UINT16)desc.numMips,
		.Format				= FormatToDxgi(desc.format).resourceFormat,
		.SampleDesc			= {.Count = desc.numSamples, .Quality = 0 },
		.Layout				= D3D12_TEXTURE_LAYOUT_UNKNOWN,
		.Flags				= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
	};

	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = FormatToDxgi(desc.format).rtvFormat;
	clearValue.DepthStencil.Depth = desc.clearDepth;
	clearValue.DepthStencil.Stencil = desc.clearStencil;

	ID3D12Resource* resource{ nullptr };
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	assert_succeeded(m_dxDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&resourceDesc, D3D12_RESOURCE_STATE_COMMON, &clearValue, IID_PPV_ARGS(&resource)));

	SetDebugName(resource, desc.name);

	// Create descriptors and derived views
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = GetDSVFormat(FormatToDxgi(desc.format).resourceFormat);

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
	m_dxDevice->CreateDepthStencilView(resource, &dsvDesc, dsvHandles[0]);

	dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
	m_dxDevice->CreateDepthStencilView(resource, &dsvDesc, dsvHandles[1]);

	auto stencilReadFormat = GetStencilFormat(FormatToDxgi(desc.format).resourceFormat);
	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
	{
		dsvHandles[2] = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		dsvHandles[3] = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		m_dxDevice->CreateDepthStencilView(resource, &dsvDesc, dsvHandles[2]);

		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		m_dxDevice->CreateDepthStencilView(resource, &dsvDesc, dsvHandles[3]);
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
	srvDesc.Format = GetDepthFormat(FormatToDxgi(desc.format).resourceFormat);
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
	m_dxDevice->CreateShaderResourceView(resource, &srvDesc, depthSrvHandle);

	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
	{
		stencilSrvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		srvDesc.Format = stencilReadFormat;
		srvDesc.Texture2D.PlaneSlice = (srvDesc.Format == DXGI_FORMAT_X32_TYPELESS_G8X24_UINT) ? 1 : 0;
		m_dxDevice->CreateShaderResourceView(resource, &srvDesc, stencilSrvHandle);
	}

	const uint8_t planeCount = GetFormatPlaneCount(FormatToDxgi(desc.format).resourceFormat);

	initialState = ResourceState::Common;

	auto descExt = DepthBufferDescExt{}
		.SetResource(resource)
		.SetUsageState(ResourceState::DepthRead | ResourceState::DepthWrite)
		.SetPlaneCount(planeCount)
		.SetDsvHandles(dsvHandles)
		.SetDepthSrvHandle(depthSrvHandle)
		.SetStencilSrvHandle(stencilSrvHandle);

	return Make<DepthBufferData>(descExt);
}


wil::com_ptr<IPlatformData> GraphicsDevice::CreateGpuBufferData(GpuBufferDesc& desc, ResourceState& initialState)
{
	wil::com_ptr<D3D12MA::Allocation> allocation = CreateGpuBuffer(desc, initialState);
	ID3D12Resource* pResource = allocation->GetResource();

	GpuBufferDescExt descExt{
		.resource		= pResource,
		.allocation		= allocation.get()
	};

	const size_t bufferSize = desc.elementCount * desc.elementSize;

	if (desc.resourceType == ResourceType::ByteAddressBuffer || desc.resourceType == ResourceType::IndirectArgsBuffer)
	{
		auto srvDesc = D3D12_SHADER_RESOURCE_VIEW_DESC{
			.Format						= DXGI_FORMAT_R32_TYPELESS,
			.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER,
			.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Buffer	= {
				.NumElements	= (uint32_t)bufferSize / 4,
				.Flags			= D3D12_BUFFER_SRV_FLAG_RAW
			}
		};

		auto srvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_dxDevice->CreateShaderResourceView(pResource, &srvDesc, srvHandle);

		auto uavDesc = D3D12_UNORDERED_ACCESS_VIEW_DESC{
			.Format			= DXGI_FORMAT_R32_TYPELESS,
			.ViewDimension	= D3D12_UAV_DIMENSION_BUFFER,
			.Buffer	= {
				.NumElements	= (uint32_t)bufferSize / 4,
				.Flags			= D3D12_BUFFER_UAV_FLAG_RAW
			}
		};
		
		auto uavHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_dxDevice->CreateUnorderedAccessView(pResource, nullptr, &uavDesc, uavHandle);

		descExt.SetSrvHandle(srvHandle);
		descExt.SetUavHandle(uavHandle);
	}

	if (desc.resourceType == ResourceType::StructuredBuffer)
	{
		auto srvDesc = D3D12_SHADER_RESOURCE_VIEW_DESC{
			.Format						= DXGI_FORMAT_UNKNOWN,
			.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER,
			.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Buffer	= {
				.NumElements			= (uint32_t)desc.elementCount,
				.StructureByteStride	= (uint32_t)desc.elementSize,
				.Flags					= D3D12_BUFFER_SRV_FLAG_NONE
			}
		};

		auto srvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_dxDevice->CreateShaderResourceView(pResource, &srvDesc, srvHandle);

		auto uavDesc = D3D12_UNORDERED_ACCESS_VIEW_DESC{
			.Format			= DXGI_FORMAT_UNKNOWN,
			.ViewDimension	= D3D12_UAV_DIMENSION_BUFFER,
			.Buffer = {
				.NumElements			= (uint32_t)desc.elementCount,
				.StructureByteStride	= (uint32_t)desc.elementSize,
				.CounterOffsetInBytes	= 0,
				.Flags					= D3D12_BUFFER_UAV_FLAG_NONE
			}
		};

		auto uavHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_dxDevice->CreateUnorderedAccessView(pResource, nullptr, &uavDesc, uavHandle);

		descExt.SetSrvHandle(srvHandle);
		descExt.SetUavHandle(uavHandle);
	}

	if (desc.resourceType == ResourceType::TypedBuffer)
	{
		auto srvDesc = D3D12_SHADER_RESOURCE_VIEW_DESC{
			.Format						= FormatToDxgi(desc.format).resourceFormat,
			.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER,
			.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Buffer = {
				.NumElements	= (uint32_t)desc.elementCount,
				.Flags			= D3D12_BUFFER_SRV_FLAG_NONE
			}
		};

		auto srvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_dxDevice->CreateShaderResourceView(pResource, &srvDesc, srvHandle);

		auto uavDesc = D3D12_UNORDERED_ACCESS_VIEW_DESC{
			.Format			= FormatToDxgi(desc.format).resourceFormat,
			.ViewDimension	= D3D12_UAV_DIMENSION_BUFFER,
			.Buffer = {
				.NumElements	= (uint32_t)desc.elementCount,
				.Flags			= D3D12_BUFFER_UAV_FLAG_NONE
			}
		};

		auto uavHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_dxDevice->CreateUnorderedAccessView(pResource, nullptr, &uavDesc, uavHandle);

		descExt.SetSrvHandle(srvHandle);
		descExt.SetUavHandle(uavHandle);
	}

	return Make<GpuBufferData>(desc, descExt);
}


wil::com_ptr<IPlatformData> GraphicsDevice::CreateRootSignatureData(RootSignatureDesc& desc)
{
	std::vector<D3D12_ROOT_PARAMETER1> d3d12RootParameters;
	
	auto exitGuard = sg::make_scope_guard([&]()
		{
			for (auto& param : d3d12RootParameters)
			{
				if (param.DescriptorTable.NumDescriptorRanges > 0)
				{
					delete[] param.DescriptorTable.pDescriptorRanges;
				}
			}
		});

	// Build merged list of root parameters
	std::vector<RootParameter> rootParameters;
	for (const auto& rootParameterSet : desc.rootParameters)
	{
		rootParameters.insert(rootParameters.end(), rootParameterSet.begin(), rootParameterSet.end());
	}

	// Build DX12 root parameter descriptions
	for (const auto& rootParameter : rootParameters)
	{
		if (rootParameter.parameterType == RootParameterType::RootConstants)
		{
			D3D12_ROOT_PARAMETER1& param = d3d12RootParameters.emplace_back();
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
			D3D12_ROOT_PARAMETER1& param = d3d12RootParameters.emplace_back();
			param.ParameterType = RootParameterTypeToDX12(rootParameter.parameterType);
			param.ShaderVisibility = ShaderStageToDX12(rootParameter.shaderVisibility);
			param.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
			param.Descriptor.RegisterSpace = rootParameter.registerSpace;
			param.Descriptor.ShaderRegister = rootParameter.startRegister;
		}
		else if (rootParameter.parameterType == RootParameterType::Table)
		{
			D3D12_ROOT_PARAMETER1& param = d3d12RootParameters.emplace_back();
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			param.ShaderVisibility = ShaderStageToDX12(rootParameter.shaderVisibility);
			
			const uint32_t numRanges = (uint32_t)rootParameter.table.size();
			param.DescriptorTable.NumDescriptorRanges = numRanges;
			D3D12_DESCRIPTOR_RANGE1* pRanges = new D3D12_DESCRIPTOR_RANGE1[numRanges];
			for (uint32_t i = 0; i < numRanges; ++i)
			{
				D3D12_DESCRIPTOR_RANGE1& d3d12Range = pRanges[i];
				const DescriptorRange& range = rootParameter.table[i];
				d3d12Range.RangeType = DescriptorTypeToDX12(range.descriptorType);
				d3d12Range.NumDescriptors = range.numDescriptors;
				d3d12Range.BaseShaderRegister = range.startRegister;
				d3d12Range.RegisterSpace = rootParameter.registerSpace;
				d3d12Range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			}
			param.DescriptorTable.pDescriptorRanges = pRanges;
		}
	}

	auto rootSignatureDesc = D3D12_VERSIONED_ROOT_SIGNATURE_DESC{
		.Version = D3D_ROOT_SIGNATURE_VERSION_1_1,
		.Desc_1_1 = {
			.NumParameters		= (uint32_t)d3d12RootParameters.size(),
			.pParameters		= d3d12RootParameters.data(),
			.NumStaticSamplers	= 0,
			.pStaticSamplers	= nullptr,
			.Flags				= RootSignatureFlagsToDX12(desc.flags)
		}
	};

	uint32_t descriptorTableBitmap{ 0 };
	uint32_t samplerTableBitmap{ 0 };
	std::vector<uint32_t> descriptorTableSize;
	descriptorTableSize.reserve(16);

	// Calculate hash
	size_t hashCode = Utility::HashState(&rootSignatureDesc.Version);
	hashCode = Utility::HashState(&rootSignatureDesc.Desc_1_1.Flags, 1, hashCode);

	for (uint32_t param = 0; param < rootSignatureDesc.Desc_1_1.NumParameters; ++param)
	{
		const D3D12_ROOT_PARAMETER1& rootParam = rootSignatureDesc.Desc_1_1.pParameters[param];
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

	ID3D12RootSignature** ppRootSignature{ nullptr };
	ID3D12RootSignature* pRootSignature{ nullptr };
	bool firstCompile = false;
	{
		lock_guard<mutex> CS(m_rootSignatureMutex);
		auto iter = m_rootSignatureHashMap.find(hashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == m_rootSignatureHashMap.end())
		{
			ppRootSignature = m_rootSignatureHashMap[hashCode].addressof();
			firstCompile = true;
		}
		else 
		{
			ppRootSignature = iter->second.addressof();
		}
	}

	if (firstCompile)
	{
		wil::com_ptr<ID3DBlob> pOutBlob, pErrorBlob;

		assert_succeeded(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &pOutBlob, &pErrorBlob));

		assert_succeeded(m_dxDevice->CreateRootSignature(0, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(),
			IID_PPV_ARGS(&pRootSignature)));

		SetDebugName(pRootSignature, desc.name);

		m_rootSignatureHashMap[hashCode].attach(pRootSignature);
		assert(*ppRootSignature == pRootSignature);
	}
	else
	{
		while (*ppRootSignature == nullptr)
		{
			this_thread::yield();
		}
		pRootSignature = *ppRootSignature;
	}

	RootSignatureDescExt rootSignatureDescExt{
		.rootSignature			= pRootSignature,
		.descriptorTableBitmap	= descriptorTableBitmap,
		.samplerTableBitmap		= samplerTableBitmap,
		.descriptorTableSize	= descriptorTableSize
	};

	return Make<RootSignatureData>(rootSignatureDescExt);
}


wil::com_ptr<IPlatformData> GraphicsDevice::CreateGraphicsPSOData(GraphicsPSODesc& desc)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3d12Desc{};
	d3d12Desc.NodeMask = 1;
	d3d12Desc.SampleMask = desc.sampleMask;
	d3d12Desc.InputLayout.NumElements = 0;

	// Blend state
	d3d12Desc.BlendState.AlphaToCoverageEnable = desc.blendState.alphaToCoverageEnable ? TRUE : FALSE;
	d3d12Desc.BlendState.IndependentBlendEnable = desc.blendState.independentBlendEnable ? TRUE : FALSE;

	for (uint32_t i = 0; i < 8; ++i)
	{
		auto& rtDesc = d3d12Desc.BlendState.RenderTarget[i];

		rtDesc.BlendEnable = desc.blendState.renderTargetBlend[i].blendEnable ? TRUE : FALSE;
		rtDesc.LogicOpEnable = desc.blendState.renderTargetBlend[i].logicOpEnable ? TRUE : FALSE;
		rtDesc.SrcBlend = BlendToDX12(desc.blendState.renderTargetBlend[i].srcBlend);
		rtDesc.DestBlend = BlendToDX12(desc.blendState.renderTargetBlend[i].dstBlend);
		rtDesc.BlendOp = BlendOpToDX12(desc.blendState.renderTargetBlend[i].blendOp);
		rtDesc.SrcBlendAlpha = BlendToDX12(desc.blendState.renderTargetBlend[i].srcBlendAlpha);
		rtDesc.DestBlendAlpha = BlendToDX12(desc.blendState.renderTargetBlend[i].dstBlendAlpha);
		rtDesc.BlendOpAlpha = BlendOpToDX12(desc.blendState.renderTargetBlend[i].blendOpAlpha);
		rtDesc.LogicOp = LogicOpToDX12(desc.blendState.renderTargetBlend[i].logicOp);
		rtDesc.RenderTargetWriteMask = ColorWriteToDX12(desc.blendState.renderTargetBlend[i].writeMask);
	}

	// Rasterizer state
	d3d12Desc.RasterizerState.FillMode = FillModeToDX12(desc.rasterizerState.fillMode);
	d3d12Desc.RasterizerState.CullMode = CullModeToDX12(desc.rasterizerState.cullMode);
	d3d12Desc.RasterizerState.FrontCounterClockwise = desc.rasterizerState.frontCounterClockwise ? TRUE : FALSE;
	d3d12Desc.RasterizerState.DepthBias = desc.rasterizerState.depthBias;
	d3d12Desc.RasterizerState.DepthBiasClamp = desc.rasterizerState.depthBiasClamp;
	d3d12Desc.RasterizerState.SlopeScaledDepthBias = desc.rasterizerState.slopeScaledDepthBias;
	d3d12Desc.RasterizerState.DepthClipEnable = desc.rasterizerState.depthClipEnable ? TRUE : FALSE;
	d3d12Desc.RasterizerState.MultisampleEnable = desc.rasterizerState.multisampleEnable ? TRUE : FALSE;
	d3d12Desc.RasterizerState.AntialiasedLineEnable = desc.rasterizerState.antialiasedLineEnable ? TRUE : FALSE;
	d3d12Desc.RasterizerState.ForcedSampleCount = desc.rasterizerState.forcedSampleCount;
	d3d12Desc.RasterizerState.ConservativeRaster =
		static_cast<D3D12_CONSERVATIVE_RASTERIZATION_MODE>(desc.rasterizerState.conservativeRasterizationEnable ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF);

	// Depth-stencil state
	d3d12Desc.DepthStencilState.DepthEnable = desc.depthStencilState.depthEnable ? TRUE : FALSE;
	d3d12Desc.DepthStencilState.DepthWriteMask = DepthWriteToDX12(desc.depthStencilState.depthWriteMask);
	d3d12Desc.DepthStencilState.DepthFunc = ComparisonFuncToDX12(desc.depthStencilState.depthFunc);
	d3d12Desc.DepthStencilState.StencilEnable = desc.depthStencilState.stencilEnable ? TRUE : FALSE;
	d3d12Desc.DepthStencilState.StencilReadMask = desc.depthStencilState.stencilReadMask;
	d3d12Desc.DepthStencilState.StencilWriteMask = desc.depthStencilState.stencilWriteMask;
	d3d12Desc.DepthStencilState.FrontFace.StencilFailOp = StencilOpToDX12(desc.depthStencilState.frontFace.stencilFailOp);
	d3d12Desc.DepthStencilState.FrontFace.StencilDepthFailOp = StencilOpToDX12(desc.depthStencilState.frontFace.stencilDepthFailOp);
	d3d12Desc.DepthStencilState.FrontFace.StencilPassOp = StencilOpToDX12(desc.depthStencilState.frontFace.stencilPassOp);
	d3d12Desc.DepthStencilState.FrontFace.StencilFunc = ComparisonFuncToDX12(desc.depthStencilState.frontFace.stencilFunc);
	d3d12Desc.DepthStencilState.BackFace.StencilFailOp = StencilOpToDX12(desc.depthStencilState.backFace.stencilFailOp);
	d3d12Desc.DepthStencilState.BackFace.StencilDepthFailOp = StencilOpToDX12(desc.depthStencilState.backFace.stencilDepthFailOp);
	d3d12Desc.DepthStencilState.BackFace.StencilPassOp = StencilOpToDX12(desc.depthStencilState.backFace.stencilPassOp);
	d3d12Desc.DepthStencilState.BackFace.StencilFunc = ComparisonFuncToDX12(desc.depthStencilState.backFace.stencilFunc);

	// Primitive topology & primitive restart
	d3d12Desc.PrimitiveTopologyType = PrimitiveTopologyToPrimitiveTopologyTypeDX12(desc.topology);
	d3d12Desc.IBStripCutValue = IndexBufferStripCutValueToDX12(desc.indexBufferStripCut);

	// Render target formats
	const uint32_t MaxRenderTargets = 8u;
	const uint32_t numRtvs = std::min<uint32_t>(MaxRenderTargets, (uint32_t)desc.rtvFormats.size());
	for (uint32_t i = 0; i < numRtvs; ++i)
	{
		d3d12Desc.RTVFormats[i] = FormatToDxgi(desc.rtvFormats[i]).rtvFormat;
	}
	for (uint32_t i = numRtvs; i < MaxRenderTargets; ++i)
	{
		d3d12Desc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
	}
	d3d12Desc.NumRenderTargets = numRtvs;
	d3d12Desc.DSVFormat = GetDSVFormat(FormatToDxgi(desc.dsvFormat).resourceFormat);
	d3d12Desc.SampleDesc.Count = desc.msaaCount;
	d3d12Desc.SampleDesc.Quality = 0; // TODO Rework this to enable quality levels in DX12

	// Input layout
	d3d12Desc.InputLayout.NumElements = (UINT)desc.vertexElements.size();
	unique_ptr<const D3D12_INPUT_ELEMENT_DESC> d3dElements;

	if (d3d12Desc.InputLayout.NumElements > 0)
	{
		D3D12_INPUT_ELEMENT_DESC* newD3DElements = (D3D12_INPUT_ELEMENT_DESC*)malloc(sizeof(D3D12_INPUT_ELEMENT_DESC) * d3d12Desc.InputLayout.NumElements);

		for (uint32_t i = 0; i < d3d12Desc.InputLayout.NumElements; ++i)
		{
			newD3DElements[i].AlignedByteOffset = desc.vertexElements[i].alignedByteOffset;
			newD3DElements[i].Format = FormatToDxgi(desc.vertexElements[i].format).resourceFormat;
			newD3DElements[i].InputSlot = desc.vertexElements[i].inputSlot;
			newD3DElements[i].InputSlotClass = InputClassificationToDX12(desc.vertexElements[i].inputClassification);
			newD3DElements[i].InstanceDataStepRate = desc.vertexElements[i].instanceDataStepRate;
			newD3DElements[i].SemanticIndex = desc.vertexElements[i].semanticIndex;
			newD3DElements[i].SemanticName = desc.vertexElements[i].semanticName;
		}

		d3dElements.reset((const D3D12_INPUT_ELEMENT_DESC*)newD3DElements);
	}

	// TODO: Shaders

	// Root signature
	d3d12Desc.pRootSignature = GetRootSignature(desc.rootSignature);

	d3d12Desc.InputLayout.pInputElementDescs = nullptr;

	size_t hashCode = Utility::HashState(&d3d12Desc);
	hashCode = Utility::HashState(desc.vertexElements.data(), d3d12Desc.InputLayout.NumElements, hashCode);

	d3d12Desc.InputLayout.pInputElementDescs = d3dElements.get();

	// Create the PSO object
	ID3D12PipelineState** ppPipelineState = nullptr;
	ID3D12PipelineState* pPipelineState = nullptr;
	bool firstCompile = false;
	{
		lock_guard<mutex> CS(m_pipelineStateMutex);

		auto iter = m_pipelineStateMap.find(hashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == m_pipelineStateMap.end())
		{
			firstCompile = true;
			ppPipelineState = m_pipelineStateMap[hashCode].addressof();
		}
		else
		{
			ppPipelineState = iter->second.addressof();
		}
	}

	if (firstCompile)
	{
		HRESULT res = m_dxDevice->CreateGraphicsPipelineState(&d3d12Desc, IID_PPV_ARGS(&pPipelineState));
		ThrowIfFailed(res);

		SetDebugName(pPipelineState, desc.name);

		m_pipelineStateMap[hashCode].attach(pPipelineState);
	}
	else
	{
		while (*ppPipelineState == nullptr)
		{
			this_thread::yield();
		}
		pPipelineState = *ppPipelineState;
	}

	GraphicsPSODescExt descExt{ .pipelineState = pPipelineState };
	return Make<GraphicsPSOData>(descExt);
}


ColorBufferHandle GraphicsDevice::CreateColorBuffer(const ColorBufferDesc& colorBufferDesc)
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
	assert_succeeded(m_dxDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
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
	m_dxDevice->CreateRenderTargetView(resource, &rtvDesc, rtvHandle);

	// Create the shader resource view
	m_dxDevice->CreateShaderResourceView(resource, &srvDesc, srvHandle);

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

			m_dxDevice->CreateUnorderedAccessView(resource, nullptr, &uavDesc, uavHandles[i]);

			uavDesc.Texture2D.MipSlice++;
		}
	}

	const uint8_t planeCount = GetFormatPlaneCount(FormatToDxgi(colorBufferDesc.format).resourceFormat);

	auto colorBufferDescExt = ColorBufferDescExt{}
		.SetResource(resource)
		.SetUsageState(ResourceState::Common)
		.SetPlaneCount(planeCount)
		.SetPlaneCount(planeCount)
		.SetRtvHandle(rtvHandle)
		.SetSrvHandle(srvHandle)
		.SetUavHandles(uavHandles);

	return Make<ColorBuffer12>(colorBufferDesc, colorBufferDescExt);
}


DepthBufferHandle GraphicsDevice::CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc)
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

	ID3D12Resource* resource{ nullptr };
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	assert_succeeded(m_dxDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&resourceDesc, D3D12_RESOURCE_STATE_COMMON, &clearValue, IID_PPV_ARGS(&resource)));

	SetDebugName(resource, depthBufferDesc.name);

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
	m_dxDevice->CreateDepthStencilView(resource, &dsvDesc, dsvHandles[0]);

	dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
	m_dxDevice->CreateDepthStencilView(resource, &dsvDesc, dsvHandles[1]);

	auto stencilReadFormat = GetStencilFormat(FormatToDxgi(depthBufferDesc.format).resourceFormat);
	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
	{
		dsvHandles[2] = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		dsvHandles[3] = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		m_dxDevice->CreateDepthStencilView(resource, &dsvDesc, dsvHandles[2]);

		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		m_dxDevice->CreateDepthStencilView(resource, &dsvDesc, dsvHandles[3]);
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
	m_dxDevice->CreateShaderResourceView(resource, &srvDesc, depthSrvHandle);

	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
	{
		stencilSrvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		srvDesc.Format = stencilReadFormat;
		srvDesc.Texture2D.PlaneSlice = (srvDesc.Format == DXGI_FORMAT_X32_TYPELESS_G8X24_UINT) ? 1 : 0;
		m_dxDevice->CreateShaderResourceView(resource, &srvDesc, stencilSrvHandle);
	}

	const uint8_t planeCount = GetFormatPlaneCount(FormatToDxgi(depthBufferDesc.format).resourceFormat);

	auto depthBufferDescExt = DepthBufferDescExt{}
		.SetResource(resource)
		.SetUsageState(ResourceState::DepthRead | ResourceState::DepthWrite)
		.SetPlaneCount(planeCount)
		.SetDsvHandles(dsvHandles)
		.SetDepthSrvHandle(depthSrvHandle)
		.SetStencilSrvHandle(stencilSrvHandle);

	return Make<DepthBuffer12>(depthBufferDesc, depthBufferDescExt);
}


GpuBufferHandle GraphicsDevice::CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc)
{
	ResourceState initialState = ResourceState::GenericRead;

	wil::com_ptr<D3D12MA::Allocation> allocation = CreateGpuBuffer(gpuBufferDesc, initialState);
	ID3D12Resource* pResource = allocation->GetResource();

	GpuBufferDescExt gpuBufferDescExt{
		.resource		= pResource,
		.allocation		= allocation.get()
	};

	const size_t bufferSize = gpuBufferDesc.elementCount * gpuBufferDesc.elementSize;

	if (gpuBufferDesc.resourceType == ResourceType::ByteAddressBuffer || gpuBufferDesc.resourceType == ResourceType::IndirectArgsBuffer)
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{
			.Format						= DXGI_FORMAT_R32_TYPELESS,
			.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER,
			.Shader4ComponentMapping	= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Buffer = {
				.NumElements	= (uint32_t)bufferSize / 4,
				.Flags			= D3D12_BUFFER_SRV_FLAG_RAW
			}
		};

		auto srvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_dxDevice->CreateShaderResourceView(pResource, &srvDesc, srvHandle);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{
			.Format		= DXGI_FORMAT_R32_TYPELESS,
			.ViewDimension	= D3D12_UAV_DIMENSION_BUFFER,
			.Buffer = {
				.NumElements	= (uint32_t)bufferSize / 4,
				.Flags			= D3D12_BUFFER_UAV_FLAG_RAW
			}
		};

		auto uavHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_dxDevice->CreateUnorderedAccessView(pResource, nullptr, &uavDesc, uavHandle);

		gpuBufferDescExt.SetSrvHandle(srvHandle);
		gpuBufferDescExt.SetUavHandle(uavHandle);
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

		auto srvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_dxDevice->CreateShaderResourceView(pResource, &srvDesc, srvHandle);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{
			.Format			= DXGI_FORMAT_UNKNOWN,
			.ViewDimension	= D3D12_UAV_DIMENSION_BUFFER,
			.Buffer = {
				.NumElements			= (uint32_t)gpuBufferDesc.elementCount,
				.StructureByteStride	= (uint32_t)gpuBufferDesc.elementSize,
				.CounterOffsetInBytes	= 0,
				.Flags					= D3D12_BUFFER_UAV_FLAG_NONE
			}
		};

		auto uavHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_dxDevice->CreateUnorderedAccessView(pResource, nullptr, &uavDesc, uavHandle);

		gpuBufferDescExt.SetSrvHandle(srvHandle);
		gpuBufferDescExt.SetUavHandle(uavHandle);
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

		auto srvHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_dxDevice->CreateShaderResourceView(pResource, &srvDesc, srvHandle);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{
			.Format			= FormatToDxgi(gpuBufferDesc.format).resourceFormat,
			.ViewDimension	= D3D12_UAV_DIMENSION_BUFFER,
			.Buffer = {
				.NumElements	= (uint32_t)gpuBufferDesc.elementCount,
				.Flags			= D3D12_BUFFER_UAV_FLAG_NONE
			}
		};

		auto uavHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_dxDevice->CreateUnorderedAccessView(pResource, nullptr, &uavDesc, uavHandle);

		gpuBufferDescExt.SetSrvHandle(srvHandle);
		gpuBufferDescExt.SetUavHandle(uavHandle);
	}

	return Make<GpuBuffer12>(gpuBufferDesc, gpuBufferDescExt);
}


void GraphicsDevice::CreateResources()
{
	if (m_desc.enableValidation)
	{
		InstallDebugCallback();
	}

	// Create descriptor allocators
	m_descriptorAllocators[0] = make_unique<DescriptorAllocator>(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_descriptorAllocators[1] = make_unique<DescriptorAllocator>(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	m_descriptorAllocators[2] = make_unique<DescriptorAllocator>(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_descriptorAllocators[3] = make_unique<DescriptorAllocator>(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	m_caps = make_unique<DeviceCaps>();
	ReadCaps();
	m_caps->LogCaps();
}


void GraphicsDevice::InstallDebugCallback()
{
	if (SUCCEEDED(m_dxDevice->QueryInterface(IID_PPV_ARGS(&m_dxInfoQueue))))
	{
		// Suppress whole categories of messages
		//D3D12_MESSAGE_CATEGORY Categories[] = {};

		// Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID denyIds[] =
		{
			// This occurs when there are uninitialized descriptors in a descriptor table, even when a
			// shader does not access the missing descriptors.  I find this is common when switching
			// shader permutations and not wanting to change much code to reorder resources.
			D3D12_MESSAGE_ID_INVALID_DESCRIPTOR_HANDLE,

			// Triggered when a shader does not export all color components of a render target, such as
			// when only writing RGB to an R10G10B10A2 buffer, ignoring alpha.
			D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_PS_OUTPUT_RT_OUTPUT_MISMATCH,

			// This occurs when a descriptor table is unbound even when a shader does not access the missing
			// descriptors.  This is common with a root signature shared between disparate shaders that
			// don't all need the same types of resources.
			D3D12_MESSAGE_ID_COMMAND_LIST_DESCRIPTOR_TABLE_NOT_SET,

			D3D12_MESSAGE_ID_COPY_DESCRIPTORS_INVALID_RANGES,

			// Silence complaints about shaders not being signed by DXIL.dll.  We don't care about this.
			D3D12_MESSAGE_ID_NON_RETAIL_SHADER_MODEL_WONT_VALIDATE,

			// RESOURCE_BARRIER_DUPLICATE_SUBRESOURCE_TRANSITIONS
			(D3D12_MESSAGE_ID)1008,
		};

		D3D12_INFO_QUEUE_FILTER newFilter = {};
		//newFilter.DenyList.NumCategories = _countof(Categories);
		//newFilter.DenyList.pCategoryList = Categories;
		newFilter.DenyList.NumSeverities = _countof(severities);
		newFilter.DenyList.pSeverityList = severities;
		newFilter.DenyList.NumIDs = _countof(denyIds);
		newFilter.DenyList.pIDList = denyIds;

#ifdef _DEBUG
		m_dxInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		m_dxInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
#endif
		m_dxInfoQueue->PushStorageFilter(&newFilter);
		m_dxInfoQueue->RegisterMessageCallback(DebugMessageCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &m_callbackCookie);
	}
}


void GraphicsDevice::ReadCaps()
{
	const D3D_FEATURE_LEVEL minFeatureLevel{ D3D_FEATURE_LEVEL_12_0 };
	const D3D_SHADER_MODEL maxShaderModel{ D3D_SHADER_MODEL_6_8 };

	m_caps->ReadFullCaps(m_dxDevice.get(), minFeatureLevel, maxShaderModel);

	// TODO
	//if (g_graphicsDeviceOptions.logDeviceFeatures)
	if (false)
	{
		m_caps->LogCaps();
	}
}


D3D12_CPU_DESCRIPTOR_HANDLE GraphicsDevice::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count)
{
	return m_descriptorAllocators[type]->Allocate(m_dxDevice.get(), count);
}


wil::com_ptr<D3D12MA::Allocation> GraphicsDevice::CreateGpuBuffer(const GpuBufferDesc& desc, ResourceState& initialState)
{ 
	initialState = ResourceState::GenericRead;

	const UINT64 bufferSize = desc.elementSize * desc.elementCount;

	D3D12_HEAP_TYPE heapType = GetHeapType(desc.memoryAccess);
	D3D12_RESOURCE_FLAGS flags = (desc.bAllowUnorderedAccess || IsUnorderedAccessType(desc.resourceType)) 
		? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS 
		: D3D12_RESOURCE_FLAG_NONE;


	D3D12_RESOURCE_DESC resourceDesc{
		.Dimension			= D3D12_RESOURCE_DIMENSION_BUFFER,
		.Alignment			= 0,
		.Width				= bufferSize,
		.Height				= 1,
		.DepthOrArraySize	= 1,
		.MipLevels			= 1,
		.Format				= FormatToDxgi(desc.format).resourceFormat,
		.SampleDesc			= { .Count = 1, .Quality = 0 },
		.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags				= flags
	};
	
	auto allocationDesc = D3D12MA::ALLOCATION_DESC{
		.HeapType = heapType
	};

	wil::com_ptr<D3D12MA::Allocation> allocation;
	HRESULT hr = m_d3d12maAllocator->CreateResource(
		&allocationDesc,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		NULL,
		&allocation,
		IID_NULL, NULL);

	return allocation;
}


wil::com_ptr<D3D12MA::Allocation> GraphicsDevice::CreateStagingBuffer(const void* initialData, size_t numBytes) const
{
	// Create an upload buffer
	auto resourceDesc = D3D12_RESOURCE_DESC{
		.Dimension	= D3D12_RESOURCE_DIMENSION_BUFFER,
		.Alignment			= 0,
		.Width				= numBytes,
		.Height				= 1,
		.DepthOrArraySize	= 1,
		.MipLevels			= 1,
		.Format				= DXGI_FORMAT_UNKNOWN,
		.SampleDesc			= {.Count = 1, .Quality = 0 },
		.Layout				= D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		.Flags				= D3D12_RESOURCE_FLAG_NONE
	};

	auto allocationDesc = D3D12MA::ALLOCATION_DESC{	.HeapType = D3D12_HEAP_TYPE_UPLOAD };

	wil::com_ptr<D3D12MA::Allocation> allocation;
	HRESULT hr = GetD3D12GraphicsDevice()->GetAllocator()->CreateResource(
		&allocationDesc,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		&allocation,
		IID_NULL, nullptr);

	auto resource = allocation->GetResource();
	void* mappedPtr{ nullptr };
	assert_succeeded(resource->Map(0, nullptr, &mappedPtr));

	memcpy(mappedPtr, initialData, numBytes);

	resource->Unmap(0, nullptr);

	return allocation;
}


uint8_t GraphicsDevice::GetFormatPlaneCount(DXGI_FORMAT format)
{
	uint8_t& planeCount = m_dxgiFormatPlaneCounts[format];
	if (planeCount == 0)
	{
		D3D12_FEATURE_DATA_FORMAT_INFO formatInfo{ format, 1 };
		if (FAILED(m_dxDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO, &formatInfo, sizeof(formatInfo))))
		{
			// Format not supported, store a special value in the cache to avoid querying later
			planeCount = 255;
		}
		else
		{
			// Format supported - store the plane count in the cache
			planeCount = formatInfo.PlaneCount;
		}
	}

	if (planeCount == 255)
	{
		return 0;
	}

	return planeCount;
}


GraphicsDevice* GetD3D12GraphicsDevice()
{
	return g_d3d12GraphicsDevice;
}

} // namespace Luna::DX12