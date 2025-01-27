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
#include "DepthBuffer12.h"
#include "Device12.h"
#include "DeviceManager12.h"
#include "GpuBuffer12.h"
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


template <class T>
inline ID3D12Resource* GetResource(const T& obj)
{
	const auto platformData = obj.GetPlatformData();
	wil::com_ptr<IGpuResourceData> gpuResourceData;
	assert_succeeded(platformData->QueryInterface(IID_PPV_ARGS(&gpuResourceData)));
	return gpuResourceData->GetResource();
}


inline D3D12_CPU_DESCRIPTOR_HANDLE GetRTV(const ColorBuffer& colorBuffer)
{
	const auto platformData = colorBuffer.GetPlatformData();
	wil::com_ptr<IColorBufferData> colorBufferData;
	assert_succeeded(platformData->QueryInterface(IID_PPV_ARGS(&colorBufferData)));
	return colorBufferData->GetRTV();
}


inline D3D12_CPU_DESCRIPTOR_HANDLE GetSRV(const ColorBuffer& colorBuffer)
{
	const auto platformData = colorBuffer.GetPlatformData();
	wil::com_ptr<IColorBufferData> colorBufferData;
	assert_succeeded(platformData->QueryInterface(IID_PPV_ARGS(&colorBufferData)));
	return colorBufferData->GetSRV();
}


inline D3D12_CPU_DESCRIPTOR_HANDLE GetUAV(const ColorBuffer& colorBuffer, uint32_t index)
{
	const auto platformData = colorBuffer.GetPlatformData();
	wil::com_ptr<IColorBufferData> colorBufferData;
	assert_succeeded(platformData->QueryInterface(IID_PPV_ARGS(&colorBufferData)));
	return colorBufferData->GetUAV(index);
}


inline D3D12_CPU_DESCRIPTOR_HANDLE GetDSV(const DepthBuffer& depthBuffer)
{
	const auto platformData = depthBuffer.GetPlatformData();
	wil::com_ptr<IDepthBufferData> depthBufferData;
	assert_succeeded(platformData->QueryInterface(IID_PPV_ARGS(&depthBufferData)));
	return depthBufferData->GetDSV();
}


CommandContext12::~CommandContext12()
{
	if (m_commandList != nullptr)
	{
		m_commandList->Release();
		m_commandList = nullptr;
	}
}


void CommandContext12::BeginEvent(const string& label)
{
#if ENABLE_D3D12_DEBUG_MARKERS
	::PIXBeginEvent(m_commandList, 0, label.c_str());
#endif
}


void CommandContext12::EndEvent()
{
#if ENABLE_D3D12_DEBUG_MARKERS
	::PIXEndEvent(m_commandList);
#endif
}


void CommandContext12::SetMarker(const string& label)
{
#if ENABLE_D3D12_DEBUG_MARKERS
	::PIXSetMarker(m_commandList, 0, label.c_str());
#endif
}


void CommandContext12::Reset()
{
	// We only call Reset() on previously freed contexts.  The command list persists, but we must
	// request a new allocator.
	assert(m_commandList != nullptr && m_currentAllocator == nullptr);
	m_currentAllocator = GetD3D12DeviceManager()->GetQueue(m_type).RequestAllocator();
	m_commandList->Reset(m_currentAllocator, nullptr);

	m_curGraphicsRootSignature = nullptr;
	m_curComputeRootSignature = nullptr;
	m_curPipelineState = nullptr;
	m_numBarriersToFlush = 0;

	BindDescriptorHeaps();
}


void CommandContext12::Initialize()
{
	GetD3D12DeviceManager()->CreateNewCommandList(m_type, &m_commandList, &m_currentAllocator);
}


uint64_t CommandContext12::Finish(bool bWaitForCompletion)
{
	assert(m_type == CommandListType::Direct || m_type == CommandListType::Compute);

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

	Queue& cmdQueue = deviceManager->GetQueue(m_type);

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


void CommandContext12::TransitionResource(ColorBuffer& colorBuffer, ResourceState newState, bool bFlushImmediate)
{
	TransitionResource_Internal(static_cast<GpuResource&>(colorBuffer), newState, bFlushImmediate);
}


void CommandContext12::TransitionResource(DepthBuffer& depthBuffer, ResourceState newState, bool bFlushImmediate)
{
	TransitionResource_Internal(static_cast<GpuResource&>(depthBuffer), newState, bFlushImmediate);
}


void CommandContext12::TransitionResource(GpuBuffer& gpuBuffer, ResourceState newState, bool bFlushImmediate)
{
	TransitionResource_Internal(static_cast<GpuResource&>(gpuBuffer), newState, bFlushImmediate);
}


void CommandContext12::InsertUAVBarrier(ColorBuffer& colorBuffer, bool bFlushImmediate)
{
	InsertUAVBarrier_Internal(static_cast<GpuResource&>(colorBuffer), bFlushImmediate);
}


void CommandContext12::InsertUAVBarrier(DepthBuffer& depthBuffer, bool bFlushImmediate)
{
	InsertUAVBarrier_Internal(static_cast<GpuResource&>(depthBuffer), bFlushImmediate);
}


void CommandContext12::FlushResourceBarriers()
{
	if (m_numBarriersToFlush > 0)
	{
		m_commandList->ResourceBarrier(m_numBarriersToFlush, m_resourceBarrierBuffer);
		m_numBarriersToFlush = 0;
	}
}


void CommandContext12::ClearColor(ColorBuffer& colorBuffer)
{
	FlushResourceBarriers();
	m_commandList->ClearRenderTargetView(GetRTV(colorBuffer), colorBuffer.GetClearColor().GetPtr(), 0, nullptr);
}


void CommandContext12::ClearColor(ColorBuffer& colorBuffer, Color clearColor)
{
	FlushResourceBarriers();
	m_commandList->ClearRenderTargetView(GetRTV(colorBuffer), clearColor.GetPtr(), 0, nullptr);
}


void CommandContext12::ClearDepth(DepthBuffer& depthBuffer)
{
	FlushResourceBarriers();
	m_commandList->ClearDepthStencilView(GetDSV(depthBuffer), D3D12_CLEAR_FLAG_DEPTH, depthBuffer.GetClearDepth(), depthBuffer.GetClearStencil(), 0, nullptr);
}


void CommandContext12::ClearStencil(DepthBuffer& depthBuffer)
{
	FlushResourceBarriers();
	m_commandList->ClearDepthStencilView(GetDSV(depthBuffer), D3D12_CLEAR_FLAG_STENCIL, depthBuffer.GetClearDepth(), depthBuffer.GetClearStencil(), 0, nullptr);
}


void CommandContext12::ClearDepthAndStencil(DepthBuffer& depthBuffer)
{
	FlushResourceBarriers();
	m_commandList->ClearDepthStencilView(GetDSV(depthBuffer), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depthBuffer.GetClearDepth(), depthBuffer.GetClearStencil(), 0, nullptr);
}


void CommandContext12::InitializeBuffer_Internal(GpuBuffer& destBuffer, const void* bufferData, size_t numBytes, size_t offset)
{ 
	wil::com_ptr<D3D12MA::Allocation> stagingAllocation = GetD3D12GraphicsDevice()->CreateStagingBuffer(bufferData, numBytes);

	// Copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
	TransitionResource(destBuffer, ResourceState::CopyDest, true);
	m_commandList->CopyBufferRegion(GetResource(destBuffer), offset, stagingAllocation->GetResource(), 0, numBytes);
	TransitionResource(destBuffer, ResourceState::GenericRead, true);

	GetD3D12DeviceManager()->ReleaseAllocation(stagingAllocation.get());
}


void CommandContext12::TransitionResource_Internal(GpuResource& gpuResource, ResourceState newState, bool bFlushImmediate)
{
	ResourceState oldState = gpuResource.GetUsageState();

	if (m_type == CommandListType::Compute)
	{
		assert(IsValidComputeResourceState(oldState));
		assert(IsValidComputeResourceState(newState));
	}

	if (oldState != newState)
	{
		assert_msg(m_numBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
		D3D12_RESOURCE_BARRIER& barrierDesc = m_resourceBarrierBuffer[m_numBarriersToFlush++];

		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrierDesc.Transition.pResource = GetResource(gpuResource);
		barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrierDesc.Transition.StateBefore = ResourceStateToDX12(oldState);
		barrierDesc.Transition.StateAfter = ResourceStateToDX12(newState);

		// Check to see if we already started the transition
		if (newState == gpuResource.GetTransitioningState())
		{
			barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
			gpuResource.SetTransitioningState(ResourceState::Undefined);
		}
		else
		{
			barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		}

		gpuResource.SetUsageState(newState);
	}
	else if (newState == ResourceState::UnorderedAccess)
	{
		InsertUAVBarrier_Internal(gpuResource, bFlushImmediate);
	}

	if (bFlushImmediate || m_numBarriersToFlush == 16)
	{
		FlushResourceBarriers();
	}
}


void CommandContext12::InsertUAVBarrier_Internal(GpuResource& gpuResource, bool bFlushImmediate)
{
	assert_msg(m_numBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
	D3D12_RESOURCE_BARRIER& barrierDesc = m_resourceBarrierBuffer[m_numBarriersToFlush++];

	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrierDesc.UAV.pResource = GetResource(gpuResource);

	if (bFlushImmediate)
	{
		FlushResourceBarriers();
	}
}


void CommandContext12::BindDescriptorHeaps()
{
	// TODO
}



} // namespace Luna::DX12