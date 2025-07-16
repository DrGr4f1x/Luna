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

namespace Luna::DX12
{

// Forward declarations
class Device;


class DepthBuffer : public IDepthBuffer
{
	friend class Device;

public:
	ID3D12Resource* GetResource() const noexcept { return m_resource.get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetDsvHandle(DepthStencilAspect depthStencilAspect) const noexcept;
	D3D12_CPU_DESCRIPTOR_HANDLE GetSrvHandle(bool depthSrv) const noexcept;

protected:
	Device* m_device{ nullptr };
	wil::com_ptr<ID3D12Resource> m_resource;
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 4> m_dsvHandles{};
	D3D12_CPU_DESCRIPTOR_HANDLE m_depthSrvHandle{};
	D3D12_CPU_DESCRIPTOR_HANDLE m_stencilSrvHandle{};
};

} // namespace Luna::DX12