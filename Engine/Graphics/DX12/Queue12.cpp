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

#include "Queue12.h"

using namespace std;
using namespace Microsoft::WRL;


namespace Luna::DX12
{

Queue::Queue(ID3D12Device* device, QueueType queueType)
	: m_type{ queueType }
	, m_allocatorPool{ device, CommandListTypeToDX12(QueueTypeToCommandListType(queueType)) }
	, m_nextFenceValue((uint64_t)queueType << 56 | 1)
	, m_lastSubmittedFenceValue{ 0 }
	, m_lastCompletedFenceValue((uint64_t)queueType << 56)
{
	auto queueDesc = D3D12_COMMAND_QUEUE_DESC{
		.Type		= CommandListTypeToDX12(QueueTypeToCommandListType(queueType)),
		.Flags		= D3D12_COMMAND_QUEUE_FLAG_NONE,
		.NodeMask	= 1
	};

	assert_succeeded(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_dxQueue)));
	SetDebugName(m_dxQueue.get(), format("{} Queue", queueType));

	assert_succeeded(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_dxFence)));
	SetDebugName(m_dxFence.get(), format("{} Queue Fence", queueType));

	m_dxFence->Signal((uint64_t)m_type << 56);
	m_lastSubmittedFenceValue = (uint64_t)m_type << 56;

	m_fenceEvent.Attach(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
}


Queue::~Queue()
{
}


uint64_t Queue::ExecuteCommandList(ID3D12CommandList* commandList)
{
	lock_guard<mutex> lockGuard(m_fenceMutex);

	assert_succeeded(((ID3D12GraphicsCommandList*)commandList)->Close());

	// Kickoff the command list
	m_dxQueue->ExecuteCommandLists(1, &commandList);

	// Signal the next fence value (with the GPU)
	m_dxQueue->Signal(m_dxFence.get(), m_nextFenceValue);
	m_lastSubmittedFenceValue = m_nextFenceValue;

	// And increment the fence value.  
	return m_nextFenceValue++;
}


ID3D12CommandAllocator* Queue::RequestAllocator()
{
	uint64_t completedFence = m_dxFence->GetCompletedValue();

	return m_allocatorPool.RequestAllocator(completedFence);
}


void Queue::DiscardAllocator(uint64_t fenceValueForReset, ID3D12CommandAllocator* allocator)
{
	m_allocatorPool.DiscardAllocator(fenceValueForReset, allocator);
}


uint64_t Queue::IncrementFence()
{
	lock_guard<mutex> lockGuard(m_fenceMutex);

	m_dxQueue->Signal(m_dxFence.get(), m_nextFenceValue);
	m_lastSubmittedFenceValue = m_nextFenceValue;

	return m_nextFenceValue++;
}


bool Queue::IsFenceComplete(uint64_t fenceValue)
{
	// Avoid querying the fence value by testing against the last one seen.
	// The max() is to protect against an unlikely race condition that could cause the last
	// completed fence value to regress.
	if (fenceValue > m_lastCompletedFenceValue)
	{
		m_lastCompletedFenceValue = std::max(m_lastCompletedFenceValue, m_dxFence->GetCompletedValue());
	}

	return fenceValue <= m_lastCompletedFenceValue;
}


void Queue::WaitForFence(uint64_t fenceValue)
{
	if (IsFenceComplete(fenceValue))
	{
		return;
	}

	// TODO:  Think about how this might affect a multi-threaded situation.  Suppose thread A
	// wants to wait for fence 100, then thread B comes along and wants to wait for 99.  If
	// the fence can only have one event set on completion, then thread B has to wait for 
	// 100 before it knows 99 is ready.  Maybe insert sequential events?
	{
		lock_guard<mutex> guard(m_eventMutex);

		ThrowIfFailed(m_dxFence->SetEventOnCompletion(fenceValue, m_fenceEvent.Get()));
		std::ignore = WaitForSingleObjectEx(m_fenceEvent.Get(), INFINITE, FALSE);
		m_lastCompletedFenceValue = fenceValue;
	}
}

} // namespace Luna::DX12