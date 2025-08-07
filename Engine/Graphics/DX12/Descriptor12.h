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

#include "Graphics\Descriptor.h"
#include "Graphics\DX12\DirectXCommon.h"

#define DESCRIPTOR_HANDLE_HEAP_TYPE_BIT_NUM   2
#define DESCRIPTOR_HANDLE_HEAP_INDEX_BIT_NUM  16
#define DESCRIPTOR_HANDLE_HEAP_OFFSET_BIT_NUM 14


namespace Luna::DX12
{

// Forward declarations
class Device;


struct DescriptorHandle2
{
	uint32_t heapType : DESCRIPTOR_HANDLE_HEAP_TYPE_BIT_NUM { 0 };
	uint32_t heapIndex : DESCRIPTOR_HANDLE_HEAP_INDEX_BIT_NUM { 0 };
	uint32_t heapOffset : DESCRIPTOR_HANDLE_HEAP_OFFSET_BIT_NUM { 0 };
	bool allocated{ false };
};


class Descriptor : public IDescriptor
{
	friend class Device;

public:
	~Descriptor() override;

	void SetDevice(Device* device) noexcept { m_device = device; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetHandleCPU() const noexcept { return m_cpuHandle; }

	void CreateConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC& desc);
	void CreateShaderResourceView(ID3D12Resource* resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& desc);
	void CreateUnorderedAccessView(ID3D12Resource* resource, const D3D12_UNORDERED_ACCESS_VIEW_DESC& desc);
	void CreateRenderTargetView(ID3D12Resource* resource, const D3D12_RENDER_TARGET_VIEW_DESC& desc);
	void CreateDepthStencilView(ID3D12Resource* resource, const D3D12_DEPTH_STENCIL_VIEW_DESC& desc);
	void CreateSampler(const D3D12_SAMPLER_DESC& desc);

	// For swapchains
	void CreateShaderResourceView(ID3D12Resource* resource);
	void CreateRenderTargetView(ID3D12Resource* resource);

private:
	void ReleaseHandle();

private:
	Device* m_device{ nullptr };
	DescriptorHandle2 m_handle{};
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHandle{ .ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN };
};

} // namespace Luna::DX12