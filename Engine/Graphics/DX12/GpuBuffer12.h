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
#include "Graphics\DX12\DirectXCommon.h"

namespace Luna::DX12
{

// Forward declarations
class Device;


class GpuBuffer : public IGpuBuffer
{
	friend class Device;

public:
	void Update(size_t sizeInBytes, const void* data) override;
	void Update(size_t sizeInBytes, size_t offset, const void* data) override;

	ID3D12Resource* GetResource() const { return m_allocation->GetResource(); }
	D3D12MA::Allocation* GetAllocation() const { return m_allocation.get(); }

	D3D12_CPU_DESCRIPTOR_HANDLE GetSrvHandle() const { return m_srvHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetUavHandle() const { return m_uavHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCbvHandle() const { return m_cbvHandle; }

	uint64_t GetGpuAddress() const;

protected:
	Device* m_device{ nullptr };
	wil::com_ptr<D3D12MA::Allocation> m_allocation;
	D3D12_CPU_DESCRIPTOR_HANDLE m_srvHandle{};
	D3D12_CPU_DESCRIPTOR_HANDLE m_uavHandle{};
	D3D12_CPU_DESCRIPTOR_HANDLE m_cbvHandle{};

	bool m_isCpuWriteable{ false };
};

} // namespace Luna::DX12