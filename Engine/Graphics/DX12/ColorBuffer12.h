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
#include "Graphics\DX12\Descriptor12.h"
#include "Graphics\DX12\DirectXCommon.h"


namespace Luna::DX12
{

// Forward declarations
class Device;


class ColorBuffer : public IColorBuffer
{
	friend class Device;

public:
	explicit ColorBuffer(Device* device);

	const IDescriptor* GetSrvDescriptor() const noexcept override { return &m_srvDescriptor; }
	const IDescriptor* GetRtvDescriptor() const noexcept override { return &m_rtvDescriptor; }
	const IDescriptor* GetUavDescriptor(uint32_t index = 0) const noexcept override;

	ID3D12Resource* GetResource() const noexcept { return m_resource.get(); }

protected:
	wil::com_ptr<ID3D12Resource> m_resource;

	// Dscriptors
	Descriptor m_srvDescriptor;
	Descriptor m_rtvDescriptor;
	std::array<Descriptor, 12> m_uavDescriptors;
};

} // namespace Luna::DX12