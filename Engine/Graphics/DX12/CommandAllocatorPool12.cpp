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

#include "CommandAllocatorPool12.h"

using namespace std;
using namespace Microsoft::WRL;


namespace Luna::DX12
{

CommandAllocatorPool::CommandAllocatorPool(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE commandListType)
	: m_dxDevice{ device }
	, m_commandListType{ commandListType }
{
}


ID3D12CommandAllocator* CommandAllocatorPool::RequestAllocator(uint64_t completedFenceValue)
{
	lock_guard<mutex> guard(m_allocatorMutex);

	ID3D12CommandAllocator* allocator{ nullptr };

	if (!m_readyAllocators.empty())
	{
		auto [fenceValue, readyAllocator] = m_readyAllocators.front();

		if (fenceValue <= completedFenceValue)
		{
			allocator = readyAllocator;
			assert_succeeded(allocator->Reset());
			m_readyAllocators.pop();
		}
	}

	// If no allocators were ready to be reused, create a new one
	if (allocator == nullptr)
	{
		assert_succeeded(m_dxDevice->CreateCommandAllocator(m_commandListType, IID_PPV_ARGS(&allocator)));

		SetDebugName(allocator, format("CommandAllocator {}", m_allocatorPool.size()));

		ComPtr<ID3D12CommandAllocator> allocatorHandle;
		allocatorHandle.Attach(allocator);

		m_allocatorPool.emplace_back(allocatorHandle);
	}

	return allocator;
}


void CommandAllocatorPool::DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* allocator)
{
	lock_guard<mutex> guard(m_allocatorMutex);

	// That fence value indicates we are free to reset the allocator
	m_readyAllocators.push(make_pair(fenceValue, allocator));
}

} // namespace Luna::DX12