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

#include "Graphics\ColorBuffer.h"
#include "Graphics\DX12\DirectXCommon.h"


namespace Luna::DX12
{

// Forward declarations
class Device;


class ColorBuffer : public IColorBuffer
{
	friend class Device;

public:
	ID3D12Resource* GetResource() const { return m_resource.get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetSrvHandle() const { return m_srvHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetRtvHandle() const { return m_rtvHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetUavHandle(uint32_t index) const;

protected:
	Device* m_device{ nullptr };
	wil::com_ptr<ID3D12Resource> m_resource;
	D3D12_CPU_DESCRIPTOR_HANDLE m_srvHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHandle;
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 12> m_uavHandles;
};

} // namespace Luna::DX12