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

#include "Graphics\PipelineState.h"
#include "Graphics\PlatformData.h"

#include "ColorBuffer12.h"
#include "DepthBuffer12.h"
#include "DescriptorSet12.h"
#include "Device12.h"
#include "DeviceManager12.h"
#include "GpuBuffer12.h"
#include "PipelineStatePool12.h"
#include "Queue12.h"
#include "ResourceSet12.h"
#include "RootSignature12.h"

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


inline D3D12_CPU_DESCRIPTOR_HANDLE GetRTV(const IColorBuffer* colorBuffer)
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle{ .ptr = colorBuffer->GetNativeObject(NativeObjectType::DX12_RTV).integer };
	return rtvHandle;
}


inline D3D12_CPU_DESCRIPTOR_HANDLE GetDSV(const IDepthBuffer* depthBuffer, DepthStencilAspect depthStencilAspect = DepthStencilAspect::ReadWrite)
{
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle{};

	switch (depthStencilAspect)
	{
	case DepthStencilAspect::ReadWrite:
		dsvHandle = D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = depthBuffer->GetNativeObject(NativeObjectType::DX12_DSV).integer };
		break;
	case DepthStencilAspect::ReadOnly:
		dsvHandle = D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = depthBuffer->GetNativeObject(NativeObjectType::DX12_DSV_ReadOnly).integer };
		break;
	case DepthStencilAspect::DepthReadOnly:
		dsvHandle = D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = depthBuffer->GetNativeObject(NativeObjectType::DX12_DSV_DepthReadOnly).integer };
		break;
	case DepthStencilAspect::StencilReadOnly:
		dsvHandle = D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = depthBuffer->GetNativeObject(NativeObjectType::DX12_DSV_StencilReadOnly).integer };
		break;
	}

	return dsvHandle;
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


void CommandContext12::TransitionResource(IGpuResource* gpuResource, ResourceState newState, bool bFlushImmediate)
{
	ResourceState oldState = gpuResource->GetUsageState();

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
		barrierDesc.Transition.pResource = gpuResource->GetNativeObject(NativeObjectType::DX12_Resource);
		barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		barrierDesc.Transition.StateBefore = ResourceStateToDX12(oldState);
		barrierDesc.Transition.StateAfter = ResourceStateToDX12(newState);

		// Check to see if we already started the transition
		if (newState == gpuResource->GetTransitioningState())
		{
			barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
			gpuResource->SetTransitioningState(ResourceState::Undefined);
		}
		else
		{
			barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		}

		gpuResource->SetUsageState(newState);
	}
	else if (newState == ResourceState::UnorderedAccess)
	{
		InsertUAVBarrier(gpuResource, bFlushImmediate);
	}

	if (bFlushImmediate || m_numBarriersToFlush == 16)
	{
		FlushResourceBarriers();
	}
}


void CommandContext12::InsertUAVBarrier(IGpuResource* gpuResource, bool bFlushImmediate)
{
	assert_msg(m_numBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
	D3D12_RESOURCE_BARRIER& barrierDesc = m_resourceBarrierBuffer[m_numBarriersToFlush++];

	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	barrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrierDesc.UAV.pResource = gpuResource->GetNativeObject(NativeObjectType::DX12_Resource);

	if (bFlushImmediate)
	{
		FlushResourceBarriers();
	}
}


void CommandContext12::FlushResourceBarriers()
{
	if (m_numBarriersToFlush > 0)
	{
		m_commandList->ResourceBarrier(m_numBarriersToFlush, m_resourceBarrierBuffer);
		m_numBarriersToFlush = 0;
	}
}


void CommandContext12::ClearUAV(IGpuBuffer* gpuBuffer)
{
	// TODO: We need to allocate a GPU descriptor, so need to implement dynamic descriptor heaps to do this.
}


void CommandContext12::ClearColor(IColorBuffer* colorBuffer)
{
	FlushResourceBarriers();
	m_commandList->ClearRenderTargetView(GetRTV(colorBuffer), colorBuffer->GetClearColor().GetPtr(), 0, nullptr);
}


void CommandContext12::ClearColor(IColorBuffer* colorBuffer, Color clearColor)
{
	FlushResourceBarriers();
	m_commandList->ClearRenderTargetView(GetRTV(colorBuffer), clearColor.GetPtr(), 0, nullptr);
}


void CommandContext12::ClearDepth(IDepthBuffer* depthBuffer)
{
	FlushResourceBarriers();
	m_commandList->ClearDepthStencilView(GetDSV(depthBuffer), D3D12_CLEAR_FLAG_DEPTH, depthBuffer->GetClearDepth(), depthBuffer->GetClearStencil(), 0, nullptr);
}


void CommandContext12::ClearStencil(IDepthBuffer* depthBuffer)
{
	FlushResourceBarriers();
	m_commandList->ClearDepthStencilView(GetDSV(depthBuffer), D3D12_CLEAR_FLAG_STENCIL, depthBuffer->GetClearDepth(), depthBuffer->GetClearStencil(), 0, nullptr);
}


void CommandContext12::ClearDepthAndStencil(IDepthBuffer* depthBuffer)
{
	FlushResourceBarriers();
	m_commandList->ClearDepthStencilView(GetDSV(depthBuffer), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depthBuffer->GetClearDepth(), depthBuffer->GetClearStencil(), 0, nullptr);
}


void CommandContext12::BeginRendering(IColorBuffer* renderTarget)
{
	assert(!m_isRendering);
	ResetRenderTargets();

	m_rtvs[0] = GetRTV(renderTarget);
	m_rtvFormats[0] = FormatToDxgi(renderTarget->GetFormat()).rtvFormat;
	m_numRtvs = 1;

	BindRenderTargets();

	m_isRendering = true;
}


void CommandContext12::BeginRendering(IColorBuffer* renderTarget, IDepthBuffer* depthTarget, DepthStencilAspect depthStencilAspect)
{
	assert(!m_isRendering);
	ResetRenderTargets();

	m_rtvs[0] = GetRTV(renderTarget);
	m_rtvFormats[0] = FormatToDxgi(renderTarget->GetFormat()).rtvFormat;
	m_numRtvs = 1;

	m_dsv = GetDSV(depthTarget, depthStencilAspect);
	m_dsvFormat = FormatToDxgi(depthTarget->GetFormat()).rtvFormat;
	m_hasDsv = true;

	BindRenderTargets();

	m_isRendering = true;
}


void CommandContext12::BeginRendering(IDepthBuffer* depthTarget, DepthStencilAspect depthStencilAspect)
{
	assert(!m_isRendering);
	ResetRenderTargets();

	m_dsv = GetDSV(depthTarget, depthStencilAspect);
	m_dsvFormat = FormatToDxgi(depthTarget->GetFormat()).rtvFormat;
	m_hasDsv = true;

	BindRenderTargets();

	m_isRendering = true;
}


void CommandContext12::BeginRendering(std::span<IColorBuffer*> renderTargets)
{
	assert(!m_isRendering);
	assert(renderTargets.size() <= 8);

	ResetRenderTargets();

	uint32_t i = 0;
	for (IColorBuffer* renderTarget : renderTargets)
	{
		m_rtvs[i] = GetRTV(renderTarget);
		m_rtvFormats[i] = FormatToDxgi(renderTarget->GetFormat()).rtvFormat;
		++i;
	}
	m_numRtvs = (uint32_t)renderTargets.size();

	BindRenderTargets();

	m_isRendering = true;
}


void CommandContext12::BeginRendering(std::span<IColorBuffer*> renderTargets, IDepthBuffer* depthTarget, DepthStencilAspect depthStencilAspect)
{
	assert(!m_isRendering);
	assert(renderTargets.size() <= 8);

	ResetRenderTargets();

	uint32_t i = 0;
	for (IColorBuffer* renderTarget : renderTargets)
	{
		m_rtvs[i] = GetRTV(renderTarget);
		m_rtvFormats[i] = FormatToDxgi(renderTarget->GetFormat()).rtvFormat;
		++i;
	}
	m_numRtvs = (uint32_t)renderTargets.size();

	m_dsv = GetDSV(depthTarget, depthStencilAspect);
	m_dsvFormat = FormatToDxgi(depthTarget->GetFormat()).rtvFormat;
	m_hasDsv = true;

	BindRenderTargets();

	m_isRendering = true;
}


void CommandContext12::EndRendering()
{
	assert(m_isRendering);
	m_isRendering = false;
}


void CommandContext12::SetRootSignature(IRootSignature* rootSignature)
{
	assert(m_type == CommandListType::Direct || m_type == CommandListType::Compute);

	if (m_type == CommandListType::Direct)
	{
		m_computeRootSignature = nullptr;
		ID3D12RootSignature* d3d12RootSignature = rootSignature->GetNativeObject(NativeObjectType::DX12_RootSignature);
		if (m_graphicsRootSignature == d3d12RootSignature)
		{
			return;
		}

		m_commandList->SetGraphicsRootSignature(d3d12RootSignature);
		m_graphicsRootSignature = d3d12RootSignature;
	}
	else
	{
		m_graphicsRootSignature = nullptr;
		ID3D12RootSignature* d3d12RootSignature = rootSignature->GetNativeObject(NativeObjectType::DX12_RootSignature);
		if (m_computeRootSignature == d3d12RootSignature)
		{
			return;
		}

		m_commandList->SetComputeRootSignature(d3d12RootSignature);
		m_computeRootSignature = d3d12RootSignature;
	}
}


void CommandContext12::SetGraphicsPipeline(GraphicsPipelineState& graphicsPipeline)
{
	m_computePipelineState = nullptr;

	// TODO: Pass handle in as function parameter
	ID3D12PipelineState* graphicsPSO = GetD3D12PipelineStatePool()->GetPipelineState(graphicsPipeline.GetHandle().get());

	if (m_graphicsPipelineState != graphicsPSO)
	{
		m_commandList->SetPipelineState(graphicsPSO);
		m_graphicsPipelineState = graphicsPSO;
	}

	// TODO: Add getter for primitive topology to PipelineStatePool
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


void CommandContext12::SetDescriptors(uint32_t rootIndex, IDescriptorSet* descriptorSet)
{
	DescriptorSetHandle descriptorSetHandle{ descriptorSet };
	SetDescriptors_Internal(rootIndex, descriptorSetHandle.query<IDescriptorSet12>().get());
}


void CommandContext12::SetResources(IResourceSet* resourceSet)
{
	// TODO: Need to rework this.  Should use a single shader-visible heap (of each type) for all descriptors.
	// See the MSDN docs on SetDescriptorHeaps.  Should only set descriptor heaps once per frame.
	ID3D12DescriptorHeap* heaps[] = {
		g_userDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].GetHeapPointer(),
		g_userDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER].GetHeapPointer()
	};
	m_commandList->SetDescriptorHeaps(2, heaps);

	ResourceSetHandle resourceSetHandle{ resourceSet };
	wil::com_ptr<IResourceSet12> resourceSet12 = resourceSetHandle.query<IResourceSet12>();
	for (uint32_t i = 0; i < resourceSet->GetNumDescriptorSets(); ++i)
	{
		SetDescriptors_Internal(i, resourceSet12->GetDescriptorSet(i));
	}
}


void CommandContext12::SetIndexBuffer(const IGpuBuffer* gpuBuffer)
{
	const bool is16Bit = gpuBuffer->GetElementSize() == sizeof(uint16_t);
	D3D12_INDEX_BUFFER_VIEW ibv{
		.BufferLocation		= gpuBuffer->GetNativeObject(NativeObjectType::DX12_GpuVirtualAddress).integer,
		.SizeInBytes		= (uint32_t)gpuBuffer->GetSize(),
		.Format				= is16Bit ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT,
	};
	m_commandList->IASetIndexBuffer(&ibv);
}


void CommandContext12::SetVertexBuffer(uint32_t slot, const IGpuBuffer* gpuBuffer)
{
	D3D12_VERTEX_BUFFER_VIEW vbv{
		.BufferLocation		= gpuBuffer->GetNativeObject(NativeObjectType::DX12_GpuVirtualAddress).integer,
		.SizeInBytes		= (uint32_t)gpuBuffer->GetSize(),
		.StrideInBytes		= (uint32_t)gpuBuffer->GetElementSize()
	};
	m_commandList->IASetVertexBuffers(slot, 1, &vbv);
}


void CommandContext12::DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount,
	uint32_t startVertexLocation, uint32_t startInstanceLocation)
{
	FlushResourceBarriers();
	// TODO
	//m_dynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_commandList);
	//m_dynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_commandList);
	m_commandList->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}


void CommandContext12::DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation,
	int32_t baseVertexLocation, uint32_t startInstanceLocation)
{
	FlushResourceBarriers();
	// TODO
	//m_dynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_commandList);
	//m_dynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_commandList);
	m_commandList->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}


void CommandContext12::InitializeBuffer_Internal(IGpuBuffer* destBuffer, const void* bufferData, size_t numBytes, size_t offset)
{ 
	wil::com_ptr<D3D12MA::Allocation> stagingAllocation = GetD3D12GraphicsDevice()->CreateStagingBuffer(bufferData, numBytes);

	// Copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
	TransitionResource(destBuffer, ResourceState::CopyDest, true);
	m_commandList->CopyBufferRegion(destBuffer->GetNativeObject(NativeObjectType::DX12_Resource), offset, stagingAllocation->GetResource(), 0, numBytes);
	TransitionResource(destBuffer, ResourceState::GenericRead, true);

	GetD3D12DeviceManager()->ReleaseAllocation(stagingAllocation.get());
}


void CommandContext12::SetDescriptors_Internal(uint32_t rootIndex, IDescriptorSet12* descriptorSet)
{
	assert(m_type == CommandListType::Direct || m_type == CommandListType::Compute);

	if (!descriptorSet->HasDescriptors() && !descriptorSet->IsRootBuffer())
		return;

	if (descriptorSet->IsDirty())
		descriptorSet->Update();

	auto gpuDescriptor = descriptorSet->GetGpuDescriptor();
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
	else if (descriptorSet->GetGpuAddress() != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
	{
		const D3D12_GPU_VIRTUAL_ADDRESS gpuFinalAddress = descriptorSet->GetGpuAddress() + descriptorSet->GetDynamicOffset();
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


void CommandContext12::BindDescriptorHeaps()
{
	// TODO
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