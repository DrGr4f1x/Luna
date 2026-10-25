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

DescriptorHeap::DescriptorHeap(DescriptorHeapType type)
{
	m_type = type;
}


DescriptorHandle DescriptorHeap::AllocateDescriptors(uint32_t numDescriptors)
{
	assert_msg(HasAvailableSpace(numDescriptors), "Descriptor Heap out of space.  Increase heap size.");
	DescriptorHandle ret = m_nextFreeHandle;
	m_nextFreeHandle += numDescriptors * m_descriptorSize;
	m_numFreeDescriptors -= numDescriptors;
	return ret;
}


void DescriptorHeap::Initialize()
{
	D3D12_DESCRIPTOR_HEAP_TYPE heapType = m_type == DescriptorHeapType::Resource ?
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV :
		D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;

	m_descriptorSize = GetDescriptorHandleIncrementSize(heapType);
	m_numFreeDescriptors = m_numDescriptors;
	m_firstHandle = DescriptorHandle(m_heap->GetCPUDescriptorHandleForHeapStart(), m_heap->GetGPUDescriptorHandleForHeapStart());
	m_nextFreeHandle = m_firstHandle;
}

} // namespace Luna::DX12