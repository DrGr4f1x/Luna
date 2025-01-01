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

#include "CommandContext12.h"

#include "ColorBuffer12.h"
#include "DeviceManager12.h"
#include "Queue12.h"

#if ENABLE_D3D12_DEBUG_MARKERS
#include <pix3.h>
#endif

using namespace std;


namespace Luna::DX12
{

static bool IsValidComputeResourceState(ResourceState state)
{
	switch (state)
	{
	case ResourceState::ShaderResource:
	case ResourceState::UnorderedAccess:
	case ResourceState::CopyDest:
	case ResourceState::CopySource:
		return true;

	default:
		return false;
	}
}


ContextState::~ContextState()
{
	if (m_commandList)
	{
		m_commandList->Release();
		m_commandList = nullptr;
	}
}

void ContextState::BeginEvent(const string& label)
{
#if ENABLE_D3D12_DEBUG_MARKERS
	::PIXBeginEvent(m_commandList, 0, label.c_str());
#endif
}


void ContextState::EndEvent()
{
#if ENABLE_D3D12_DEBUG_MARKERS
	::PIXEndEvent(m_commandList);
#endif
}


void ContextState::SetMarker(const string& label)
{
#if ENABLE_D3D12_DEBUG_MARKERS
	::PIXSetMarker(m_commandList, 0, label.c_str());
#endif
}


void ContextState::Reset()
{
	// We only call Reset() on previously freed contexts.  The command list persists, but we must
	// request a new allocator.
	assert(m_commandList != nullptr && m_currentAllocator == nullptr);
	m_currentAllocator = GetD3D12DeviceManager()->GetQueue(type).RequestAllocator();
	m_commandList->Reset(m_currentAllocator, nullptr);

	m_curGraphicsRootSignature = nullptr;
	m_curComputeRootSignature = nullptr;
	m_curPipelineState = nullptr;
	m_numBarriersToFlush = 0;

	BindDescriptorHeaps();
}


void ContextState::Initialize()
{
	GetD3D12DeviceManager()->CreateNewCommandList(type, &m_commandList, &m_currentAllocator);
}


uint64_t ContextState::Finish(bool bWaitForCompletion)
{
	assert(type == CommandListType::Direct || type == CommandListType::Compute);

	FlushResourceBarriers();

	// TODO
#if 0
	if (m_ID.length() > 0)
	{
		EngineProfiling::EndBlock(this);
	}
#endif

	assert(m_currentAllocator != nullptr);

	if (m_bHasPendingDebugEvent)
	{
		EndEvent();
		m_bHasPendingDebugEvent = false;
	}

	auto deviceManager = GetD3D12DeviceManager();

	Queue& cmdQueue = deviceManager->GetQueue(type);

	uint64_t fenceValue = cmdQueue.ExecuteCommandList(m_commandList);
	cmdQueue.DiscardAllocator(fenceValue, m_currentAllocator);
	m_currentAllocator = nullptr;

	// TODO
	//m_cpuLinearAllocator.CleanupUsedPages(fenceValue);
	//m_gpuLinearAllocator.CleanupUsedPages(fenceValue);
	//m_dynamicViewDescriptorHeap.CleanupUsedHeaps(fenceValue);
	//m_dynamicSamplerDescriptorHeap.CleanupUsedHeaps(fenceValue);

	if (bWaitForCompletion)
	{
		deviceManager->WaitForFence(fenceValue);
	}

	return fenceValue;
}


void ContextState::TransitionResource(ColorBuffer& colorBuffer, ResourceState newState, bool bFlushImmediate)
{
	ResourceState oldState = colorBuffer.GetUsageState();

	if (type == CommandListType::Compute)
	{
		assert(IsValidComputeResourceState(oldState));
		assert(IsValidComputeResourceState(newState));
	}

	if (oldState != newState)
	{
		assert_msg(m_numBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
		D3D12_RESOURCE_BARRIER& barrierDesc = m_resourceBarrierBuffer[m_numBarriersToFlush++];

		// TODO: put a wrapper function around this
		auto platformData = colorBuffer.GetPlatformData();
		wil::com_ptr<IColorBufferData> colorBufferData;
		assert_succeeded(platformData->QueryInterface(IID_PPV_ARGS(&colorBufferData)));

		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrierDesc.Transition.pResource = colorBufferData->GetResource();
		barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrierDesc.Transition.StateBefore = ResourceStateToDX12(oldState);
		barrierDesc.Transition.StateAfter = ResourceStateToDX12(newState);

		// Check to see if we already started the transition
		if (newState == colorBuffer.GetTransitioningState())
		{
			barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
			colorBuffer.SetTransitioningState(ResourceState::Undefined);
		}
		else
		{
			barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		}

		colorBuffer.SetUsageState(newState);
	}
	else if (newState == ResourceState::UnorderedAccess)
	{
		InsertUAVBarrier(colorBuffer, bFlushImmediate);
	}

	if (bFlushImmediate || m_numBarriersToFlush == 16)
	{
		FlushResourceBarriers();
	}
}


void ContextState::InsertUAVBarrier(ColorBuffer& colorBuffer, bool bFlushImmediate)
{
	assert_msg(m_numBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
	D3D12_RESOURCE_BARRIER& barrierDesc = m_resourceBarrierBuffer[m_numBarriersToFlush++];

	auto platformData = colorBuffer.GetPlatformData();
	wil::com_ptr<IColorBufferData> colorBufferData;
	assert_succeeded(platformData->QueryInterface(IID_PPV_ARGS(&colorBufferData)));

	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrierDesc.UAV.pResource = colorBufferData->GetResource();

	if (bFlushImmediate)
	{
		FlushResourceBarriers();
	}
}


void ContextState::FlushResourceBarriers()
{
	if (m_numBarriersToFlush > 0)
	{
		m_commandList->ResourceBarrier(m_numBarriersToFlush, m_resourceBarrierBuffer);
		m_numBarriersToFlush = 0;
	}
}


void ContextState::ClearColor(ColorBuffer& colorBuffer)
{
	FlushResourceBarriers();

	auto platformData = colorBuffer.GetPlatformData();
	wil::com_ptr<IColorBufferData> colorBufferData;
	assert_succeeded(platformData->QueryInterface(IID_PPV_ARGS(&colorBufferData)));

	m_commandList->ClearRenderTargetView(colorBufferData->GetRTV(), colorBuffer.GetClearColor().GetPtr(), 0, nullptr);
}


void ContextState::ClearColor(ColorBuffer& colorBuffer, Color clearColor)
{
	FlushResourceBarriers();

	auto platformData = colorBuffer.GetPlatformData();
	wil::com_ptr<IColorBufferData> colorBufferData;
	assert_succeeded(platformData->QueryInterface(IID_PPV_ARGS(&colorBufferData)));

	m_commandList->ClearRenderTargetView(colorBufferData->GetRTV(), clearColor.GetPtr(), 0, nullptr);
}


void ContextState::BindDescriptorHeaps()
{
	// TODO
}


ComputeContext::ComputeContext()
{
	m_state.type = CommandListType::Compute;
}


uint64_t ComputeContext::Finish(bool bWaitForCompletion)
{
	uint64_t fenceValue = m_state.Finish(bWaitForCompletion);

	GetD3D12DeviceManager()->FreeContext(this);

	return fenceValue;
}


GraphicsContext::GraphicsContext()
{
	m_state.type = CommandListType::Direct;
}


uint64_t GraphicsContext::Finish(bool bWaitForCompletion)
{
	uint64_t fenceValue = m_state.Finish(bWaitForCompletion);

	GetD3D12DeviceManager()->FreeContext(this);

	return fenceValue;
}

} // namespace Luna::DX12