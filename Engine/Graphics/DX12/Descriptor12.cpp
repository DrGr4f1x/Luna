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

#include "Descriptor12.h"

#include "Device12.h"


namespace Luna::DX12
{

DescriptorType DescriptorTypeFromShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC& desc)
{
	// Buffers
	if (desc.ViewDimension == D3D12_SRV_DIMENSION_BUFFER)
	{
		if (desc.Buffer.Flags == D3D12_BUFFER_SRV_FLAG_RAW)
		{
			return DescriptorType::RawBufferSRV;
		}

		if (desc.Buffer.StructureByteStride != 0)
		{
			return DescriptorType::StructuredBufferSRV;
		}
		else
		{
			return DescriptorType::TypedBufferSRV;
		}

	}
	else if (desc.ViewDimension >= D3D12_SRV_DIMENSION_TEXTURE1D && desc.ViewDimension <= D3D12_SRV_DIMENSION_TEXTURECUBEARRAY)
	{
		return DescriptorType::TextureSRV;
	}
	else if (desc.ViewDimension == D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE)
	{
		return DescriptorType::RayTracingAccelStruct;
	}

	return DescriptorType::None;
}

DescriptorType DescriptorTypeFromUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc)
{
	// Buffers
	if (desc.ViewDimension == D3D12_UAV_DIMENSION_BUFFER)
	{
		if (desc.Buffer.Flags == D3D12_BUFFER_UAV_FLAG_RAW)
		{
			return DescriptorType::RawBufferUAV;
		}

		if (desc.Buffer.StructureByteStride != 0)
		{
			return DescriptorType::StructuredBufferUAV;
		}
		else
		{
			return DescriptorType::TypedBufferUAV;
		}

	}
	else if (desc.ViewDimension >= D3D12_UAV_DIMENSION_TEXTURE1D && desc.ViewDimension <= D3D12_UAV_DIMENSION_TEXTURE3D)
	{
		return DescriptorType::TextureUAV;
	}
	
	return DescriptorType::None;
}


Descriptor::~Descriptor()
{
	ReleaseHandle();
}


void Descriptor::CreateConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc)
{
	assert(m_device != nullptr);

	ReleaseHandle();

	m_handle = m_device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_cpuHandle = m_device->GetDescriptorHandleCPU(m_handle);
	m_gpuAddress = desc.BufferLocation;
	m_type = DescriptorType::ConstantBuffer;

	m_device->GetD3D12Device()->CreateConstantBufferView(&desc, m_cpuHandle);
}


void Descriptor::CreateShaderResourceView(ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc)
{
	assert(m_device != nullptr);

	ReleaseHandle();

	m_handle = m_device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_cpuHandle = m_device->GetDescriptorHandleCPU(m_handle);

	// If the resource is a buffer, get its GPU virtual address (this is NULL for non-buffer resources)
	m_gpuAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
	if (resource->GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		m_gpuAddress = resource->GetGPUVirtualAddress();
	}
	
	m_type = DescriptorTypeFromShaderResourceView(desc);

	m_device->GetD3D12Device()->CreateShaderResourceView(resource, &desc, m_cpuHandle);
}


void Descriptor::CreateUnorderedAccessView(ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc)
{
	assert(m_device != nullptr);

	ReleaseHandle();

	m_handle = m_device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_cpuHandle = m_device->GetDescriptorHandleCPU(m_handle);
	
	// If the resource is a buffer, get its GPU virtual address (this is NULL for non-buffer resources)
	m_gpuAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
	if (resource->GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		m_gpuAddress = resource->GetGPUVirtualAddress();
	}
	
	m_type = DescriptorTypeFromUnorderedAccessView(desc);

	m_device->GetD3D12Device()->CreateUnorderedAccessView(resource, nullptr, &desc, m_cpuHandle);
}


void Descriptor::CreateRenderTargetView(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC& desc)
{
	assert(m_device != nullptr);

	ReleaseHandle();

	m_handle = m_device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_cpuHandle = m_device->GetDescriptorHandleCPU(m_handle);
	
	// If the resource is a buffer, get its GPU virtual address (this is NULL for non-buffer resources)
	m_gpuAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
	if (resource->GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		m_gpuAddress = resource->GetGPUVirtualAddress();
	}
	
	m_type = DescriptorType::None; // TODO: Descriptor types for RTVs?

	m_device->GetD3D12Device()->CreateRenderTargetView(resource, &desc, m_cpuHandle);
}


void Descriptor::CreateDepthStencilView(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc)
{
	assert(m_device != nullptr);

	ReleaseHandle();

	m_handle = m_device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_cpuHandle = m_device->GetDescriptorHandleCPU(m_handle);

	// If the resource is a buffer, get its GPU virtual address (this is NULL for non-buffer resources)
	m_gpuAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
	if (resource->GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		m_gpuAddress = resource->GetGPUVirtualAddress();
	}

	m_type = DescriptorType::None; // TODO: Descriptor types for DSVs?

	m_device->GetD3D12Device()->CreateDepthStencilView(resource, &desc, m_cpuHandle);
}


void Descriptor::CreateSampler(const D3D12_SAMPLER_DESC& desc)
{
	assert(m_device != nullptr);

	ReleaseHandle();

	m_handle = m_device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	m_cpuHandle = m_device->GetDescriptorHandleCPU(m_handle);
	m_gpuAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
	m_type = DescriptorType::Sampler;

	m_device->GetD3D12Device()->CreateSampler(&desc, m_cpuHandle);
}


void Descriptor::CreateShaderResourceView(ID3D12Resource* resource)
{
	assert(m_device != nullptr);

	ReleaseHandle();

	m_handle = m_device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_cpuHandle = m_device->GetDescriptorHandleCPU(m_handle);
	
	// If the resource is a buffer, get its GPU virtual address (this is NULL for non-buffer resources)
	m_gpuAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
	if (resource->GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		m_gpuAddress = resource->GetGPUVirtualAddress();
	}
	
	m_type = DescriptorType::TextureSRV; // TODO: Get the Desc from the ID3D12Resource to determine the actual type

	m_device->GetD3D12Device()->CreateShaderResourceView(resource, nullptr, m_cpuHandle);
}


void Descriptor::CreateRenderTargetView(ID3D12Resource* resource)
{
	assert(m_device != nullptr);

	ReleaseHandle();

	m_handle = m_device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_cpuHandle = m_device->GetDescriptorHandleCPU(m_handle);
	
	// If the resource is a buffer, get its GPU virtual address (this is NULL for non-buffer resources)
	m_gpuAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
	if (resource->GetDesc().Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		m_gpuAddress = resource->GetGPUVirtualAddress();
	}

	m_type = DescriptorType::None; // TODO: Descriptor types for RTVs?

	m_device->GetD3D12Device()->CreateRenderTargetView(resource, nullptr, m_cpuHandle);
}


void Descriptor::ReleaseHandle()
{
	assert(m_device != nullptr);
	if (m_handle.allocated)
	{
		m_device->FreeDescriptorHandle(m_handle);
		m_handle = DescriptorHandle2{};
	}
}

} // namespace Luna::DX12