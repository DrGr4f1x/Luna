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

	bool HasAvailableSpace(DescriptorHeapType type, uint32_t count) const;
	uint32_t GetNumFreeDescriptors(DescriptorHeapType type) const;

	ID3D12DescriptorHeap* GetHeap(DescriptorHeapType type) const;

	DescriptorHandle AllocateDescriptors(DescriptorHeapType type, uint32_t numDescriptors);

protected:
	void Initialize();

protected:

	struct HeapState
	{
		wil::com_ptr<ID3D12DescriptorHeap> heap;

		D3D12_CPU_DESCRIPTOR_HANDLE currentHandle{ D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN };
		uint32_t numDescriptors;
		uint32_t descriptorSize{ 0 };
		uint32_t numFreeDescriptors;
		DescriptorHandle firstHandle;
		DescriptorHandle nextFreeHandle;
	};

	HeapState m_resourceHeap;
	HeapState m_samplerHeap;

protected:
	HeapState& GetHeapState(DescriptorHeapType type) { return (type == DescriptorHeapType::Resource) ? m_resourceHeap : m_samplerHeap; }
	const HeapState& GetHeapState(DescriptorHeapType type) const { return (type == DescriptorHeapType::Resource) ? m_resourceHeap : m_samplerHeap; }
};

} // namespace Luna::DX12