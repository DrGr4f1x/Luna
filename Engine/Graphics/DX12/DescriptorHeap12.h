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

#include "Graphics\DescriptorHeap.h"

#include "Graphics\DX12\DescriptorHandle12.h"
#include "Graphics\DX12\DirectXCommon.h"

namespace Luna::DX12
{

class DescriptorHeap : public IDescriptorHeap
{
public:
	friend class Device;

	explicit DescriptorHeap(DescriptorHeapType type);

	bool HasAvailableSpace(uint32_t count) const { return count <= m_numFreeDescriptors; }
	uint32_t GetNumFreeDescriptors() const { return m_numFreeDescriptors; }

	ID3D12DescriptorHeap* GetHeap() const { return m_heap.get(); }

	DescriptorHandle AllocateDescriptors(uint32_t numDescriptors);

protected:
	void Initialize();

protected:
	wil::com_ptr<ID3D12DescriptorHeap> m_heap;
	D3D12_CPU_DESCRIPTOR_HANDLE m_currentHandle{ D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN };
	uint32_t m_numDescriptors;
	uint32_t m_descriptorSize{ 0 };
	uint32_t m_numFreeDescriptors;
	DescriptorHandle m_firstHandle;
	DescriptorHandle m_nextFreeHandle;
};

} // namespace Luna::DX12