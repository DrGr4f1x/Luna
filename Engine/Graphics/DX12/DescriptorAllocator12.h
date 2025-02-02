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


namespace Luna::DX12
{

class DescriptorAllocator
{
public:
	explicit DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type)
		: m_type{ type }
	{
	}

	D3D12_CPU_DESCRIPTOR_HANDLE Allocate(ID3D12Device* device, uint32_t count);

private:
	ID3D12DescriptorHeap* RequestNewHeap(ID3D12Device* device);

private:
	static constexpr uint32_t m_numDescriptorsPerHeap{ 256 };

	D3D12_DESCRIPTOR_HEAP_TYPE m_type{ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV };
	std::mutex m_allocationMutex;
	std::vector<wil::com_ptr<ID3D12DescriptorHeap>> m_descriptorHeapPool;
	ID3D12DescriptorHeap* m_currentHeap{ nullptr };
	D3D12_CPU_DESCRIPTOR_HANDLE m_currentHandle{ D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN };
	uint32_t m_descriptorSize{ 0 };
	uint32_t m_remainingFreeHandles{ 0 };
};


class DescriptorHandle
{
public:
	DescriptorHandle() noexcept = default;

	explicit DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle) noexcept
		: m_cpuHandle(cpuHandle)
	{}

	DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle) noexcept
		: m_cpuHandle(cpuHandle)
		, m_gpuHandle(gpuHandle)
	{}

	DescriptorHandle operator+(int32_t offsetScaledByDescriptorSize) const noexcept
	{
		DescriptorHandle ret = *this;
		ret += offsetScaledByDescriptorSize;
		return ret;
	}

	void operator+=(int32_t offsetScaledByDescriptorSize) noexcept
	{
		if (m_cpuHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		{
			m_cpuHandle.ptr += offsetScaledByDescriptorSize;
		}

		if (m_gpuHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		{
			m_gpuHandle.ptr += offsetScaledByDescriptorSize;
		}
	}

	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHandle() const noexcept { return m_cpuHandle; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle() const noexcept { return m_gpuHandle; }

	bool IsNull() const noexcept { return m_cpuHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; }
	bool IsShaderVisible() const noexcept { return m_gpuHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; }

private:
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHandle{ .ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN };
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuHandle{ .ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN };
};

} // namespace Luna::DX12