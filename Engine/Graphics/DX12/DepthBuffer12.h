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

#include "Graphics\DepthBuffer.h"
#include "Graphics\DX12\DirectXCommon.h"
#include "Graphics\DX12\GpuResource12.h"

using namespace Microsoft::WRL;


namespace Luna::DX12
{

struct DepthBufferDescExt
{
	DepthBufferDescExt()
	{
		for (auto& handle : dsvHandles)
		{
			handle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		}
		depthSrvHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		stencilSrvHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	ID3D12Resource* resource{ nullptr };
	ResourceState usageState{ ResourceState::Undefined };
	uint8_t planeCount{ 1 };

	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 4> dsvHandles{};
	D3D12_CPU_DESCRIPTOR_HANDLE depthSrvHandle{};
	D3D12_CPU_DESCRIPTOR_HANDLE stencilSrvHandle{};

	constexpr DepthBufferDescExt& SetResource(ID3D12Resource* value) noexcept { resource = value; return *this; }
	constexpr DepthBufferDescExt& SetUsageState(ResourceState value) noexcept { usageState = value; return *this; }
	constexpr DepthBufferDescExt& SetPlaneCount(uint8_t value) noexcept { planeCount = value; return *this; }
	DepthBufferDescExt& SetDsvHandles(const std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 4>& value) noexcept { dsvHandles = value; return *this; }
	constexpr DepthBufferDescExt& SetDepthSrvHandle(D3D12_CPU_DESCRIPTOR_HANDLE value) noexcept { depthSrvHandle = value; return *this; }
	constexpr DepthBufferDescExt& SetStencilSrvHandle(D3D12_CPU_DESCRIPTOR_HANDLE value) noexcept { stencilSrvHandle = value; return *this; }
};


class __declspec(uuid("232C36B6-60FC-4DE6-8AD5-84769D22CDFF")) IDepthBufferData : public IGpuResourceData
{
public:
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const noexcept = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDSV_DepthReadOnly() const noexcept = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDSV_StencilReadOnly() const noexcept = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDSV_ReadOnly() const noexcept = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDepthSRV() const noexcept = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetStencilSRV() const noexcept = 0;
};


class __declspec(uuid("36E0E19C-7D07-46EA-A6FE-E222A083957D")) DepthBufferData
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IDepthBufferData, IGpuResourceData, IPlatformData>>
	, NonCopyable
{
public:
	DepthBufferData(const DepthBufferDescExt& descExt) noexcept;

	ID3D12Resource* GetResource() const noexcept final { return m_resource.get(); }
	uint64_t GetGpuAddress() const noexcept override { return m_resource->GetGPUVirtualAddress(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const noexcept final { return m_dsvHandles[0]; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSV_DepthReadOnly() const noexcept final { return m_dsvHandles[1]; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSV_StencilReadOnly() const noexcept final { return m_dsvHandles[2]; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSV_ReadOnly() const noexcept final { return m_dsvHandles[3]; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetDepthSRV() const noexcept final { return m_depthSrvHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetStencilSRV() const noexcept final { return m_stencilSrvHandle; }

private:
	wil::com_ptr<ID3D12Resource> m_resource;
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 4> m_dsvHandles{};
	D3D12_CPU_DESCRIPTOR_HANDLE m_depthSrvHandle{};
	D3D12_CPU_DESCRIPTOR_HANDLE m_stencilSrvHandle{};
};

} // namespace Luna::DX12