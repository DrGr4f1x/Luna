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

#include "DescriptorAllocator12.h"

#include "Strings12.h"

using namespace std;
using namespace Microsoft::WRL;


namespace Luna::DX12
{

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocator::Allocate(ID3D12Device* device, uint32_t count)
{
	assert(count <= m_numDescriptorsPerHeap);

	if (m_currentHeap == nullptr || m_remainingFreeHandles < count)
	{
		m_currentHeap = RequestNewHeap(device);
		m_currentHandle = m_currentHeap->GetCPUDescriptorHandleForHeapStart();
		m_remainingFreeHandles = m_numDescriptorsPerHeap;

		if (m_descriptorSize == 0)
		{
			m_descriptorSize = device->GetDescriptorHandleIncrementSize(m_type);
		}
	}

	auto handle = m_currentHandle;
	m_currentHandle.ptr += count * m_descriptorSize;
	m_remainingFreeHandles -= count;
	return handle;
}


ID3D12DescriptorHeap* DescriptorAllocator::RequestNewHeap(ID3D12Device* device)
{
	lock_guard<mutex> guard{ m_allocationMutex };

	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.Type = m_type;
	desc.NumDescriptors = m_numDescriptorsPerHeap;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask = 1;

	wil::com_ptr<ID3D12DescriptorHeap> heap;
	assert_succeeded(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));

	SetDebugName(heap.get(), format("DescriptorAllocator [{}] {}", m_type, m_descriptorHeapPool.size()));
	m_descriptorHeapPool.emplace_back(heap);

	return heap.get();
}

} // namespace Luna::DX12