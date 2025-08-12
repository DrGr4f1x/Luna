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
#include "Graphics\DX12\Descriptor12.h"
#include "Graphics\DX12\DirectXCommon.h"


namespace Luna::DX12
{

// Forward declarations
class Device;


class DepthBuffer : public IDepthBuffer
{
	friend class Device;

public:
	explicit DepthBuffer(Device* device);

	const IDescriptor* GetDsvDescriptor(DepthStencilAspect aspect) const noexcept override;
	const IDescriptor* GetSrvDescriptor(bool depthSrv) const noexcept override;

	ID3D12Resource* GetResource() const noexcept { return m_resource.get(); }

protected:
	wil::com_ptr<ID3D12Resource> m_resource;
	std::array<Descriptor, 4> m_dsvDescriptors{};
	Descriptor m_depthSrvDescriptor{};
	Descriptor m_stencilSrvDescriptor{};
};

} // namespace Luna::DX12