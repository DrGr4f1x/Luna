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

	ID3D12Resource* GetResource() const noexcept { return m_resource.get(); }

	const Descriptor& GetSrvDescriptor() const noexcept { return m_srvDescriptor; }
	const Descriptor& GetRtvDescriptor() const noexcept { return m_rtvDescriptor; }
	const Descriptor& GetUavDescriptor(uint32_t index) const;

protected:
	wil::com_ptr<ID3D12Resource> m_resource;

	// Dscriptors
	Descriptor m_srvDescriptor;
	Descriptor m_rtvDescriptor;
	std::array<Descriptor, 12> m_uavDescriptors;
};

} // namespace Luna::DX12