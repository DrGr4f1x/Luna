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

#include "Graphics\DX12\DirectXCommon.h"
#include "Graphics\Texture.h"


namespace Luna::DX12
{

// Forward declarations
class Device;


class Texture : public ITexture
{
	friend class Device;

public:
	bool IsValid() const override { return m_resource != nullptr; }

	ID3D12Resource* GetResource() const { return m_resource.get(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetSrvHandle() const { return m_srvHandle; }

protected:
	Device* m_device{ nullptr };
	wil::com_ptr<ID3D12Resource> m_resource;
	D3D12_CPU_DESCRIPTOR_HANDLE m_srvHandle{ .ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN };
	std::string m_name;
};

} // namespace Luna::DX12