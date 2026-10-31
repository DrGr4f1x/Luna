//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#include "Stdafx.h"

#include "DescriptorHeap12.h"

namespace Luna::DX12
{

bool DescriptorHeap::HasAvailableSpace(DescriptorHeapType type, uint32_t count) const
{
	const HeapState& heapState = GetHeapState(type);
	return count <= heapState.numFreeDescriptors;
}


uint32_t DescriptorHeap::GetNumFreeDescriptors(DescriptorHeapType type) const
{
	return GetHeapState(type).numFreeDescriptors;
}


ID3D12DescriptorHeap* DescriptorHeap::GetHeap(DescriptorHeapType type) const
{
	return GetHeapState(type).heap.get();
}


DescriptorHandle DescriptorHeap::AllocateDescriptors(DescriptorHeapType type, uint32_t numDescriptors)
{
	assert_msg(HasAvailableSpace(type, numDescriptors), "Descriptor Heap out of space.  Increase heap size.");

	HeapState& heapState = GetHeapState(type);

	DescriptorHandle ret = heapState.nextFreeHandle;
	heapState.nextFreeHandle += numDescriptors * heapState.descriptorSize;
	heapState.numFreeDescriptors -= numDescriptors;
	return ret;
}


void DescriptorHeap::Initialize()
{
	// Resource heap
	{
		m_resourceHeap.descriptorSize = GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_resourceHeap.numFreeDescriptors = m_resourceHeap.numDescriptors;
		m_resourceHeap.firstHandle = DescriptorHandle(m_resourceHeap.heap->GetCPUDescriptorHandleForHeapStart(), m_resourceHeap.heap->GetGPUDescriptorHandleForHeapStart());
		m_resourceHeap.nextFreeHandle = m_resourceHeap.firstHandle;
	}

	// Sampler heap
	{
		m_samplerHeap.descriptorSize = GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
		m_samplerHeap.numFreeDescriptors = m_samplerHeap.numDescriptors;
		m_samplerHeap.firstHandle = DescriptorHandle(m_samplerHeap.heap->GetCPUDescriptorHandleForHeapStart(), m_samplerHeap.heap->GetGPUDescriptorHandleForHeapStart());
		m_samplerHeap.nextFreeHandle = m_samplerHeap.firstHandle;
	}
}

} // namespace Luna::DX12