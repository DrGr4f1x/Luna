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

#include "Graphics\ResourceSet.h"

#include "DeviceManager12.h"
#include "DirectXCommon.h"
#include "Queue12.h"
#include "ResourceManager12.h"

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


CommandContext12::CommandContext12(CommandListType type)
	: m_type{ type }
	, m_dynamicViewDescriptorHeap{ *this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV }
	, m_dynamicSamplerDescriptorHeap{ *this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER }
{
	ZeroMemory(m_currentDescriptorHeaps, sizeof(m_currentDescriptorHeaps));
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

	m_graphicsRootSignature = nullptr;
	m_computeRootSignature = nullptr;
	m_graphicsPipelineState = nullptr;
	m_computePipelineState = nullptr;
	m_primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	m_numBarriersToFlush = 0;

	BindDescriptorHeaps();
	ResetRenderTargets();
}


void CommandContext12::Initialize()
{
	GetD3D12DeviceManager()->CreateNewCommandList(m_type, &m_commandList, &m_currentAllocator);

	m_resourceManager = GetD3D12ResourceManager();
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
	m_dynamicViewDescriptorHeap.CleanupUsedHeaps(fenceValue);
	m_dynamicSamplerDescriptorHeap.CleanupUsedHeaps(fenceValue);

	if (bWaitForCompletion)
	{
		deviceManager->WaitForFence(fenceValue);
	}

	return fenceValue;
}


void CommandContext12::TransitionResource(ColorBuffer& colorBuffer, ResourceState newState, bool bFlushImmediate)
{
	ResourceState oldState = colorBuffer.GetUsageState();

	if (m_type == CommandListType::Compute)
	{
		assert(IsValidComputeResourceState(oldState));
		assert(IsValidComputeResourceState(newState));
	}

	auto handle = colorBuffer.GetHandle();

	ID3D12Resource* resource = m_resourceManager->GetResource(handle.get());

	TransitionResource_Internal(resource, ResourceStateToDX12(oldState), ResourceStateToDX12(newState), bFlushImmediate);

	colorBuffer.SetUsageState(newState);
}


void CommandContext12::TransitionResource(DepthBuffer& depthBuffer, ResourceState newState, bool bFlushImmediate)
{
	ResourceState oldState = depthBuffer.GetUsageState();

	if (m_type == CommandListType::Compute)
	{
		assert(IsValidComputeResourceState(oldState));
		assert(IsValidComputeResourceState(newState));
	}

	auto handle = depthBuffer.GetHandle();

	ID3D12Resource* resource = m_resourceManager->GetResource(handle.get());

	TransitionResource_Internal(resource, ResourceStateToDX12(oldState), ResourceStateToDX12(newState), bFlushImmediate);

	depthBuffer.SetUsageState(newState);
}


void CommandContext12::TransitionResource(GpuBuffer& gpuBuffer, ResourceState newState, bool bFlushImmediate)
{
	ResourceState oldState = gpuBuffer.GetUsageState();

	if (m_type == CommandListType::Compute)
	{
		assert(IsValidComputeResourceState(oldState));
		assert(IsValidComputeResourceState(newState));
	}

	auto handle = gpuBuffer.GetHandle();

	ID3D12Resource* resource = m_resourceManager->GetResource(handle.get());

	TransitionResource_Internal(resource, ResourceStateToDX12(oldState), ResourceStateToDX12(newState), bFlushImmediate);

	gpuBuffer.SetUsageState(newState);
}


void CommandContext12::FlushResourceBarriers()
{
	if (m_numBarriersToFlush > 0)
	{
		m_commandList->ResourceBarrier(m_numBarriersToFlush, m_resourceBarrierBuffer);
		m_numBarriersToFlush = 0;
	}
}


void CommandContext12::ClearUAV(GpuBuffer& gpuBuffer)
{
	// TODO: We need to allocate a GPU descriptor, so need to implement dynamic descriptor heaps to do this.
}


void CommandContext12::ClearColor(ColorBuffer& colorBuffer)
{
	FlushResourceBarriers();

	auto colorBufferHandle = colorBuffer.GetHandle();

	m_commandList->ClearRenderTargetView(m_resourceManager->GetRTV(colorBufferHandle.get()), colorBuffer.GetClearColor().GetPtr(), 0, nullptr);
}


void CommandContext12::ClearColor(ColorBuffer& colorBuffer, Color clearColor)
{
	FlushResourceBarriers();

	auto colorBufferHandle = colorBuffer.GetHandle();

	m_commandList->ClearRenderTargetView(m_resourceManager->GetRTV(colorBufferHandle.get()), clearColor.GetPtr(), 0, nullptr);
}


void CommandContext12::ClearDepth(DepthBuffer& depthBuffer)
{
	FlushResourceBarriers();

	auto handle = depthBuffer.GetHandle();

	m_commandList->ClearDepthStencilView(m_resourceManager->GetDSV(handle.get(), DepthStencilAspect::ReadWrite), D3D12_CLEAR_FLAG_DEPTH, depthBuffer.GetClearDepth(), depthBuffer.GetClearStencil(), 0, nullptr);
}


void CommandContext12::ClearStencil(DepthBuffer& depthBuffer)
{
	FlushResourceBarriers();

	auto handle = depthBuffer.GetHandle();

	m_commandList->ClearDepthStencilView(m_resourceManager->GetDSV(handle.get(), DepthStencilAspect::ReadWrite), D3D12_CLEAR_FLAG_STENCIL, depthBuffer.GetClearDepth(), depthBuffer.GetClearStencil(), 0, nullptr);
}


void CommandContext12::ClearDepthAndStencil(DepthBuffer& depthBuffer)
{
	FlushResourceBarriers();

	auto handle = depthBuffer.GetHandle();

	m_commandList->ClearDepthStencilView(m_resourceManager->GetDSV(handle.get(), DepthStencilAspect::ReadWrite), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depthBuffer.GetClearDepth(), depthBuffer.GetClearStencil(), 0, nullptr);
}


void CommandContext12::BeginRendering(ColorBuffer& renderTarget)
{
	assert(!m_isRendering);
	ResetRenderTargets();

	auto colorBufferHandle = renderTarget.GetHandle();

	m_rtvs[0] = m_resourceManager->GetRTV(colorBufferHandle.get());
	m_rtvFormats[0] = FormatToDxgi(renderTarget.GetFormat()).rtvFormat;
	m_numRtvs = 1;

	BindRenderTargets();

	m_isRendering = true;
}


void CommandContext12::BeginRendering(ColorBuffer& renderTarget, DepthBuffer& depthTarget, DepthStencilAspect depthStencilAspect)
{
	assert(!m_isRendering);
	ResetRenderTargets();

	auto colorBufferHandle = renderTarget.GetHandle();

	m_rtvs[0] = m_resourceManager->GetRTV(colorBufferHandle.get());
	m_rtvFormats[0] = FormatToDxgi(renderTarget.GetFormat()).rtvFormat;
	m_numRtvs = 1;

	auto handle = depthTarget.GetHandle();

	m_dsv = m_resourceManager->GetDSV(handle.get(), depthStencilAspect);
	m_dsvFormat = FormatToDxgi(depthTarget.GetFormat()).rtvFormat;
	m_hasDsv = true;

	BindRenderTargets();

	m_isRendering = true;
}


void CommandContext12::BeginRendering(DepthBuffer& depthTarget, DepthStencilAspect depthStencilAspect)
{
	assert(!m_isRendering);
	ResetRenderTargets();

	auto handle = depthTarget.GetHandle();

	m_dsv = m_resourceManager->GetDSV(handle.get(), depthStencilAspect);
	m_dsvFormat = FormatToDxgi(depthTarget.GetFormat()).rtvFormat;
	m_hasDsv = true;

	BindRenderTargets();

	m_isRendering = true;
}


void CommandContext12::BeginRendering(std::span<ColorBuffer> renderTargets)
{
	assert(!m_isRendering);
	assert(renderTargets.size() <= 8);

	ResetRenderTargets();

	uint32_t i = 0;
	for (const ColorBuffer& renderTarget : renderTargets)
	{
		auto colorBufferHandle = renderTarget.GetHandle();

		m_rtvs[i] = m_resourceManager->GetRTV(colorBufferHandle.get());
		m_rtvFormats[i] = FormatToDxgi(renderTarget.GetFormat()).rtvFormat;
		++i;
	}
	m_numRtvs = (uint32_t)renderTargets.size();

	BindRenderTargets();

	m_isRendering = true;
}


void CommandContext12::BeginRendering(std::span<ColorBuffer> renderTargets, DepthBuffer& depthTarget, DepthStencilAspect depthStencilAspect)
{
	assert(!m_isRendering);
	assert(renderTargets.size() <= 8);

	ResetRenderTargets();

	uint32_t i = 0;
	for (const ColorBuffer& renderTarget : renderTargets)
	{
		auto colorBufferHandle = renderTarget.GetHandle();

		m_rtvs[i] = m_resourceManager->GetRTV(colorBufferHandle.get());
		m_rtvFormats[i] = FormatToDxgi(renderTarget.GetFormat()).rtvFormat;
		++i;
	}
	m_numRtvs = (uint32_t)renderTargets.size();

	auto handle = depthTarget.GetHandle();

	m_dsv = m_resourceManager->GetDSV(handle.get(), depthStencilAspect);
	m_dsvFormat = FormatToDxgi(depthTarget.GetFormat()).rtvFormat;
	m_hasDsv = true;

	BindRenderTargets();

	m_isRendering = true;
}


void CommandContext12::EndRendering()
{
	assert(m_isRendering);
	m_isRendering = false;
}


void CommandContext12::SetRootSignature(RootSignature& rootSignature)
{
	assert(m_type == CommandListType::Direct || m_type == CommandListType::Compute);

	ID3D12RootSignature* d3d12RootSignature = m_resourceManager->GetRootSignature(rootSignature.GetHandle().get());

	if (m_type == CommandListType::Direct)
	{
		m_computeRootSignature = nullptr;
		if (m_graphicsRootSignature == d3d12RootSignature)
		{
			return;
		}

		m_commandList->SetGraphicsRootSignature(d3d12RootSignature);
		m_graphicsRootSignature = d3d12RootSignature;
		m_dynamicViewDescriptorHeap.ParseGraphicsRootSignature(rootSignature);
		m_dynamicSamplerDescriptorHeap.ParseGraphicsRootSignature(rootSignature);
	}
	else
	{
		m_graphicsRootSignature = nullptr;
		if (m_computeRootSignature == d3d12RootSignature)
		{
			return;
		}

		m_commandList->SetComputeRootSignature(d3d12RootSignature);
		m_computeRootSignature = d3d12RootSignature;
		m_dynamicViewDescriptorHeap.ParseComputeRootSignature(rootSignature);
		m_dynamicSamplerDescriptorHeap.ParseComputeRootSignature(rootSignature);
	}
}


void CommandContext12::SetGraphicsPipeline(GraphicsPipelineState& graphicsPipeline)
{
	m_computePipelineState = nullptr;

	// TODO: Pass handle in as function parameter
	ID3D12PipelineState* graphicsPSO = m_resourceManager->GetGraphicsPipelineState(graphicsPipeline.GetHandle().get());

	if (m_graphicsPipelineState != graphicsPSO)
	{
		m_commandList->SetPipelineState(graphicsPSO);
		m_graphicsPipelineState = graphicsPSO;
	}

	// TODO: Add getter for primitive topology to PipelineStateManager
	auto topology = PrimitiveTopologyToDX12(graphicsPipeline.GetPrimitiveTopology());
	if (m_primitiveTopology != topology)
	{
		m_commandList->IASetPrimitiveTopology(topology);
		m_primitiveTopology = topology;
	}
}


void CommandContext12::SetViewport(float x, float y, float w, float h, float minDepth, float maxDepth)
{ 
	D3D12_VIEWPORT viewport{
		.TopLeftX	= x,
		.TopLeftY	= y,
		.Width		= w,
		.Height		= h,
		.MinDepth	= minDepth,
		.MaxDepth	= maxDepth
	};

	m_commandList->RSSetViewports(1, &viewport);
}


void CommandContext12::SetScissor(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
{
	auto rect = CD3DX12_RECT(left, top, right, bottom);

	assert(rect.left < rect.right && rect.top < rect.bottom);

	m_commandList->RSSetScissorRects(1, &rect);
}


void CommandContext12::SetStencilRef(uint32_t stencilRef)
{
	m_commandList->OMSetStencilRef(stencilRef);
}


void CommandContext12::SetBlendFactor(Color blendFactor)
{
	m_commandList->OMSetBlendFactor(blendFactor.GetPtr());
}


void CommandContext12::SetPrimitiveTopology(PrimitiveTopology topology)
{
	const auto newTopology = PrimitiveTopologyToDX12(topology);
	if (m_primitiveTopology != newTopology)
	{
		m_commandList->IASetPrimitiveTopology(newTopology);
		m_primitiveTopology = newTopology;
	}
}


void CommandContext12::SetConstantArray(uint32_t rootIndex, uint32_t numConstants, const void* constants, uint32_t offset)
{
	assert(m_type == CommandListType::Direct || m_type == CommandListType::Compute);
	if (m_type == CommandListType::Direct)
	{
		m_commandList->SetGraphicsRoot32BitConstants(rootIndex, numConstants, constants, offset);
	}
	else
	{
		m_commandList->SetComputeRoot32BitConstants(rootIndex, numConstants, constants, offset);
	}
}


void CommandContext12::SetConstant(uint32_t rootIndex, uint32_t offset, DWParam val)
{
	assert(m_type == CommandListType::Direct || m_type == CommandListType::Compute);
	if (m_type == CommandListType::Direct)
	{
		m_commandList->SetGraphicsRoot32BitConstant(rootIndex, get<uint32_t>(val.value), offset);
	}
	else
	{
		m_commandList->SetComputeRoot32BitConstant(rootIndex, get<uint32_t>(val.value), offset);
	}
}


void CommandContext12::SetConstants(uint32_t rootIndex, DWParam x)
{
	assert(m_type == CommandListType::Direct || m_type == CommandListType::Compute);
	if (m_type == CommandListType::Direct)
	{
		m_commandList->SetGraphicsRoot32BitConstant(rootIndex, get<uint32_t>(x.value), 0);
	}
	else
	{
		m_commandList->SetComputeRoot32BitConstant(rootIndex, get<uint32_t>(x.value), 0);
	}
}


void CommandContext12::SetConstants(uint32_t rootIndex, DWParam x, DWParam y)
{
	assert(m_type == CommandListType::Direct || m_type == CommandListType::Compute);
	if (m_type == CommandListType::Direct)
	{
		m_commandList->SetGraphicsRoot32BitConstant(rootIndex, get<uint32_t>(x.value), 0);
		m_commandList->SetGraphicsRoot32BitConstant(rootIndex, get<uint32_t>(y.value), 1);
	}
	else
	{
		m_commandList->SetComputeRoot32BitConstant(rootIndex, get<uint32_t>(x.value), 0);
		m_commandList->SetComputeRoot32BitConstant(rootIndex, get<uint32_t>(y.value), 1);
	}
}


void CommandContext12::SetConstants(uint32_t rootIndex, DWParam x, DWParam y, DWParam z)
{
	assert(m_type == CommandListType::Direct || m_type == CommandListType::Compute);
	if (m_type == CommandListType::Direct)
	{
		m_commandList->SetGraphicsRoot32BitConstant(rootIndex, get<uint32_t>(x.value), 0);
		m_commandList->SetGraphicsRoot32BitConstant(rootIndex, get<uint32_t>(y.value), 1);
		m_commandList->SetGraphicsRoot32BitConstant(rootIndex, get<uint32_t>(z.value), 2);
	}
	else
	{
		m_commandList->SetComputeRoot32BitConstant(rootIndex, get<uint32_t>(x.value), 0);
		m_commandList->SetComputeRoot32BitConstant(rootIndex, get<uint32_t>(y.value), 1);
		m_commandList->SetComputeRoot32BitConstant(rootIndex, get<uint32_t>(z.value), 2);
	}
}


void CommandContext12::SetConstants(uint32_t rootIndex, DWParam x, DWParam y, DWParam z, DWParam w)
{
	assert(m_type == CommandListType::Direct || m_type == CommandListType::Compute);
	if (m_type == CommandListType::Direct)
	{
		m_commandList->SetGraphicsRoot32BitConstant(rootIndex, get<uint32_t>(x.value), 0);
		m_commandList->SetGraphicsRoot32BitConstant(rootIndex, get<uint32_t>(y.value), 1);
		m_commandList->SetGraphicsRoot32BitConstant(rootIndex, get<uint32_t>(z.value), 2);
		m_commandList->SetGraphicsRoot32BitConstant(rootIndex, get<uint32_t>(w.value), 3);
	}
	else
	{
		m_commandList->SetComputeRoot32BitConstant(rootIndex, get<uint32_t>(x.value), 0);
		m_commandList->SetComputeRoot32BitConstant(rootIndex, get<uint32_t>(y.value), 1);
		m_commandList->SetComputeRoot32BitConstant(rootIndex, get<uint32_t>(z.value), 2);
		m_commandList->SetComputeRoot32BitConstant(rootIndex, get<uint32_t>(w.value), 3);
	}
}


void CommandContext12::SetConstantBuffer(uint32_t rootIndex, const GpuBuffer& gpuBuffer)
{
	assert(m_type == CommandListType::Direct || m_type == CommandListType::Compute);

	auto handle = gpuBuffer.GetHandle();
	uint64_t gpuAddress = m_resourceManager->GetGpuAddress(handle.get());

	if (m_type == CommandListType::Direct)
	{
		m_commandList->SetGraphicsRootConstantBufferView(rootIndex, gpuAddress);
	}
	else
	{
		m_commandList->SetComputeRootConstantBufferView(rootIndex, gpuAddress);
	}
}


void CommandContext12::SetDescriptors(uint32_t rootIndex, DescriptorSet& descriptorSet)
{
	SetDescriptors_Internal(rootIndex, descriptorSet.GetHandle().get());
}


void CommandContext12::SetResources(ResourceSet& resourceSet)
{
	// TODO: Need to rework this.  Should use a single shader-visible heap (of each type) for all descriptors.
	// See the MSDN docs on SetDescriptorHeaps.  Should only set descriptor heaps once per frame.
	ID3D12DescriptorHeap* heaps[] = {
		g_userDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].GetHeapPointer(),
		g_userDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER].GetHeapPointer()
	};
	m_commandList->SetDescriptorHeaps(2, heaps);

	const uint32_t numDescriptorSets = resourceSet.GetNumDescriptorSets();
	for (uint32_t i = 0; i < numDescriptorSets; ++i)
	{
		SetDescriptors_Internal(i, resourceSet[i].GetHandle().get());
	}
}


void CommandContext12::SetSRV(uint32_t rootIndex, uint32_t offset, const ColorBuffer& colorBuffer)
{
	auto handle = colorBuffer.GetHandle();
	auto descriptor = m_resourceManager->GetSRV(handle.get(), true);

	SetDynamicDescriptors_Internal(rootIndex, offset, 1, &descriptor);
}


void CommandContext12::SetSRV(uint32_t rootIndex, uint32_t offset, const DepthBuffer& depthBuffer, bool depthSrv)
{
	auto handle = depthBuffer.GetHandle();
	auto descriptor = m_resourceManager->GetSRV(handle.get(), depthSrv);

	SetDynamicDescriptors_Internal(rootIndex, offset, 1, &descriptor);
}


void CommandContext12::SetSRV(uint32_t rootIndex, uint32_t offset, const GpuBuffer& gpuBuffer)
{
	auto handle = gpuBuffer.GetHandle();
	auto descriptor = m_resourceManager->GetSRV(handle.get(), true);

	SetDynamicDescriptors_Internal(rootIndex, offset, 1, &descriptor);
}


void CommandContext12::SetUAV(uint32_t rootIndex, uint32_t offset, const ColorBuffer& colorBuffer)
{
	auto handle = colorBuffer.GetHandle();
	// TODO: Need a UAV index parameter
	auto descriptor = m_resourceManager->GetUAV(handle.get(), 0);

	SetDynamicDescriptors_Internal(rootIndex, offset, 1, &descriptor);
}


void CommandContext12::SetUAV(uint32_t rootIndex, uint32_t offset, const DepthBuffer& depthBuffer)
{
	// TODO: support this
	assert(false);
}


void CommandContext12::SetUAV(uint32_t rootIndex, uint32_t offset, const GpuBuffer& gpuBuffer)
{
	auto handle = gpuBuffer.GetHandle();
	auto descriptor = m_resourceManager->GetUAV(handle.get());

	SetDynamicDescriptors_Internal(rootIndex, offset, 1, &descriptor);
}


void CommandContext12::SetCBV(uint32_t rootIndex, uint32_t offset, const GpuBuffer& gpuBuffer)
{
	auto handle = gpuBuffer.GetHandle();
	auto descriptor = m_resourceManager->GetCBV(handle.get());

	SetDynamicDescriptors_Internal(rootIndex, offset, 1, &descriptor);
}


void CommandContext12::SetIndexBuffer(const GpuBuffer& gpuBuffer)
{
	auto handle = gpuBuffer.GetHandle();

	const bool is16Bit = m_resourceManager->GetElementSize(handle.get()) == sizeof(uint16_t);
	D3D12_INDEX_BUFFER_VIEW ibv{
		.BufferLocation		= m_resourceManager->GetGpuAddress(handle.get()),
		.SizeInBytes		= (uint32_t)gpuBuffer.GetSize(),
		.Format				= is16Bit ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT,
	};
	m_commandList->IASetIndexBuffer(&ibv);
}


void CommandContext12::SetVertexBuffer(uint32_t slot, const GpuBuffer& gpuBuffer)
{
	auto handle = gpuBuffer.GetHandle();

	D3D12_VERTEX_BUFFER_VIEW vbv{
		.BufferLocation		= m_resourceManager->GetGpuAddress(handle.get()),
		.SizeInBytes		= (uint32_t)gpuBuffer.GetSize(),
		.StrideInBytes		= (uint32_t)gpuBuffer.GetElementSize()
	};
	m_commandList->IASetVertexBuffers(slot, 1, &vbv);
}


void CommandContext12::DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount,
	uint32_t startVertexLocation, uint32_t startInstanceLocation)
{
	FlushResourceBarriers();
	m_dynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_commandList);
	m_dynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_commandList);
	m_commandList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}


void CommandContext12::DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation,
	int32_t baseVertexLocation, uint32_t startInstanceLocation)
{
	FlushResourceBarriers();
	m_dynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_commandList);
	m_dynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_commandList);
	m_commandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}


void CommandContext12::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap* heapPtr)
{
	if (m_currentDescriptorHeaps[type] != heapPtr)
	{
		m_currentDescriptorHeaps[type] = heapPtr;
		BindDescriptorHeaps();
	}
}


void CommandContext12::SetDescriptorHeaps(uint32_t heapCount, D3D12_DESCRIPTOR_HEAP_TYPE types[], ID3D12DescriptorHeap* heapPtrs[])
{
	bool anyChanged = false;
	for (uint32_t i = 0; i < heapCount; ++i)
	{
		if (m_currentDescriptorHeaps[types[i]] != heapPtrs[i])
		{
			m_currentDescriptorHeaps[types[i]] = heapPtrs[i];
			anyChanged = true;
		}
	}

	if (anyChanged)
	{
		BindDescriptorHeaps();
	}
}


void CommandContext12::TransitionResource_Internal(ID3D12Resource* resource, D3D12_RESOURCE_STATES oldState, D3D12_RESOURCE_STATES newState, bool bFlushImmediate)
{
	if (oldState != newState)
	{
		assert_msg(m_numBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
		D3D12_RESOURCE_BARRIER& barrierDesc = m_resourceBarrierBuffer[m_numBarriersToFlush++];

		barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrierDesc.Transition.pResource = resource;
		barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrierDesc.Transition.StateBefore = oldState;
		barrierDesc.Transition.StateAfter = newState;
		barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	}
	else if (newState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
	{
		InsertUAVBarrier_Internal(resource, bFlushImmediate);
	}

	if (bFlushImmediate || m_numBarriersToFlush == 16)
	{
		FlushResourceBarriers();
	}
}


void CommandContext12::InsertUAVBarrier_Internal(ID3D12Resource* resource, bool bFlushImmediate)
{
	assert_msg(m_numBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
	D3D12_RESOURCE_BARRIER& barrierDesc = m_resourceBarrierBuffer[m_numBarriersToFlush++];

	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrierDesc.UAV.pResource = resource;

	if (bFlushImmediate)
	{
		FlushResourceBarriers();
	}
}


void CommandContext12::InitializeBuffer_Internal(GpuBuffer& destBuffer, const void* bufferData, size_t numBytes, size_t offset)
{ 
	auto handle = destBuffer.GetHandle();

	auto deviceManager = GetD3D12DeviceManager();

	wil::com_ptr<D3D12MA::Allocation> stagingAllocation = CreateStagingBuffer(deviceManager->GetAllocator(), bufferData, numBytes);

	// Copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
	TransitionResource(destBuffer, ResourceState::CopyDest, true);
	m_commandList->CopyBufferRegion(m_resourceManager->GetResource(handle.get()), offset, stagingAllocation->GetResource(), 0, numBytes);
	TransitionResource(destBuffer, ResourceState::GenericRead, true);

	GetD3D12DeviceManager()->ReleaseAllocation(stagingAllocation.get());
}


void CommandContext12::SetDescriptors_Internal(uint32_t rootIndex, ResourceHandleType* resourceHandle)
{
	assert(m_type == CommandListType::Direct || m_type == CommandListType::Compute);

	if (!m_resourceManager->HasBindableDescriptors(resourceHandle))
		return;

	// Copy descriptors
	m_resourceManager->UpdateGpuDescriptors(resourceHandle);

	auto gpuDescriptor = m_resourceManager->GetGpuDescriptorHandle(resourceHandle);
	if (gpuDescriptor.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
	{
		if (m_type == CommandListType::Direct)
		{
			m_commandList->SetGraphicsRootDescriptorTable(rootIndex, gpuDescriptor);
		}
		else
		{
			m_commandList->SetComputeRootDescriptorTable(rootIndex, gpuDescriptor);
		}
	}
	else if (m_resourceManager->GetDescriptorSetGpuAddress(resourceHandle) != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
	{
		const D3D12_GPU_VIRTUAL_ADDRESS gpuFinalAddress = m_resourceManager->GetGpuAddressWithOffset(resourceHandle);
		if (m_type == CommandListType::Direct)
		{
			m_commandList->SetGraphicsRootConstantBufferView(rootIndex, gpuFinalAddress);
		}
		else
		{
			m_commandList->SetComputeRootConstantBufferView(rootIndex, gpuFinalAddress);
		}
	}
}


void CommandContext12::SetDynamicDescriptors_Internal(uint32_t rootIndex, uint32_t offset, uint32_t numDescriptors, const D3D12_CPU_DESCRIPTOR_HANDLE handles[])
{
	if (m_type == CommandListType::Direct)
	{
		m_dynamicViewDescriptorHeap.SetGraphicsDescriptorHandles(rootIndex, offset, numDescriptors, handles);
	}
	else
	{
		m_dynamicViewDescriptorHeap.SetComputeDescriptorHandles(rootIndex, offset, numDescriptors, handles);
	}
}


void CommandContext12::BindDescriptorHeaps()
{
	uint32_t nonNullHeaps = 0;
	ID3D12DescriptorHeap* heapsToBind[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
	for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		auto heapIter = m_currentDescriptorHeaps[i];
		if (heapIter != nullptr)
		{
			heapsToBind[nonNullHeaps++] = heapIter;
		}
	}

	if (nonNullHeaps > 0)
	{
		m_commandList->SetDescriptorHeaps(nonNullHeaps, heapsToBind);
	}
}


void CommandContext12::BindRenderTargets()
{
	m_commandList->OMSetRenderTargets(
		m_numRtvs, 
		m_numRtvs > 0 ? m_rtvs.data() : nullptr, 
		false, 
		m_hasDsv ? &m_dsv : nullptr);
}


void CommandContext12::ResetRenderTargets()
{
	for (uint32_t i = 0; i < 8; ++i)
	{
		m_rtvs[i] = D3D12_CPU_DESCRIPTOR_HANDLE{};
		m_rtvFormats[i] = DXGI_FORMAT_UNKNOWN;
	}
	m_numRtvs = 0;
	m_dsv = D3D12_CPU_DESCRIPTOR_HANDLE{};
	m_dsvFormat = DXGI_FORMAT_UNKNOWN;
	m_hasDsv = false;
}

} // namespace Luna::DX12