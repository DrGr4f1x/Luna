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

#include "Graphics\DX12\Descriptor12.h"
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
	explicit Texture(Device* device);

	bool IsValid() const noexcept override { return m_resource != nullptr; }

	const IDescriptor* GetDescriptor() const override { return &m_srvDescriptor; }

	ID3D12Resource* GetResource() const noexcept { return m_resource.get(); }
	const Descriptor& GetSrvDescriptor() const noexcept { return m_srvDescriptor; }

protected:
	wil::com_ptr<ID3D12Resource> m_resource;
	Descriptor m_srvDescriptor;
	std::string m_name;
};

} // namespace Luna::DX12