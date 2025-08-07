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

	m_device->GetD3D12Device()->CreateConstantBufferView(&desc, m_cpuHandle);
}


void Descriptor::CreateShaderResourceView(ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc)
{
	assert(m_device != nullptr);

	ReleaseHandle();

	m_handle = m_device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_cpuHandle = m_device->GetDescriptorHandleCPU(m_handle);

	m_device->GetD3D12Device()->CreateShaderResourceView(resource, &desc, m_cpuHandle);
}


void Descriptor::CreateUnorderedAccessView(ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc)
{
	assert(m_device != nullptr);

	ReleaseHandle();

	m_handle = m_device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_cpuHandle = m_device->GetDescriptorHandleCPU(m_handle);

	m_device->GetD3D12Device()->CreateUnorderedAccessView(resource, nullptr, &desc, m_cpuHandle);
}


void Descriptor::CreateRenderTargetView(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC& desc)
{
	assert(m_device != nullptr);

	ReleaseHandle();

	m_handle = m_device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_cpuHandle = m_device->GetDescriptorHandleCPU(m_handle);

	m_device->GetD3D12Device()->CreateRenderTargetView(resource, &desc, m_cpuHandle);
}


void Descriptor::CreateDepthStencilView(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc)
{
	assert(m_device != nullptr);

	ReleaseHandle();

	m_handle = m_device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_cpuHandle = m_device->GetDescriptorHandleCPU(m_handle);

	m_device->GetD3D12Device()->CreateDepthStencilView(resource, &desc, m_cpuHandle);
}


void Descriptor::CreateSampler(const D3D12_SAMPLER_DESC& desc)
{
	assert(m_device != nullptr);

	ReleaseHandle();

	m_handle = m_device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	m_cpuHandle = m_device->GetDescriptorHandleCPU(m_handle);

	m_device->GetD3D12Device()->CreateSampler(&desc, m_cpuHandle);
}


void Descriptor::CreateShaderResourceView(ID3D12Resource* resource)
{
	assert(m_device != nullptr);

	ReleaseHandle();

	m_handle = m_device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_cpuHandle = m_device->GetDescriptorHandleCPU(m_handle);

	m_device->GetD3D12Device()->CreateShaderResourceView(resource, nullptr, m_cpuHandle);
}


void Descriptor::CreateRenderTargetView(ID3D12Resource* resource)
{
	assert(m_device != nullptr);

	ReleaseHandle();

	m_handle = m_device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_cpuHandle = m_device->GetDescriptorHandleCPU(m_handle);

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