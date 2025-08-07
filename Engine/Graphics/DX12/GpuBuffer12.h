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

#include "Graphics\GpuBuffer.h"
#include "Graphics\DX12\Descriptor12.h"
#include "Graphics\DX12\DirectXCommon.h"


namespace Luna::DX12
{

// Forward declarations
class Device;


class GpuBuffer : public IGpuBuffer
{
	friend class Device;

public:
	explicit GpuBuffer(Device* device);

	void Update(size_t sizeInBytes, const void* data) override;
	void Update(size_t sizeInBytes, size_t offset, const void* data) override;

	void* Map() override;
	void Unmap() override;

	ID3D12Resource* GetResource() const noexcept { return m_allocation->GetResource(); }
	D3D12MA::Allocation* GetAllocation() const noexcept { return m_allocation.get(); }

	const Descriptor& GetSrvDescriptor() const noexcept { return m_srvDescriptor; }
	const Descriptor& GetUavDescriptor() const noexcept { return m_uavDescriptor; }
	const Descriptor& GetCbvDescriptor() const noexcept { return m_cbvDescriptor; }

	uint64_t GetGpuAddress() const noexcept;

protected:
	wil::com_ptr<D3D12MA::Allocation> m_allocation;

	Descriptor m_srvDescriptor{};
	Descriptor m_uavDescriptor{};
	Descriptor m_cbvDescriptor{};

	bool m_isCpuWriteable{ false };
};

} // namespace Luna::DX12