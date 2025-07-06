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

#include "ColorBuffer12.h"
#include "DepthBuffer12.h"
#include "DescriptorSet12.h"
#include "DeviceManager12.h"
#include "DirectXCommon.h"
#include "GpuBuffer12.h"
#include "Queue12.h"
#include "RootSignature12.h"
#include "PipelineState12.h"
#include "Texture12.h"


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


inline D3D12_SUBRESOURCE_DATA TextureSubResourceDataToDX12(const TextureSubresourceData& data)
{
	D3D12_SUBRESOURCE_DATA subResourceData{
		.pData			= data.data,
		.RowPitch		= (LONG_PTR)data.rowPitch,
		.SlicePitch		= (LONG_PTR)data.slicePitch
	};

	return subResourceData;
}


CommandContext12::CommandContext12(CommandListType type)
	: m_type{ type }
	, m_dynamicViewDescriptorHeap{ *this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV }
	, m_dynamicSamplerDescriptorHeap{ *this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER }
	, m_cpuLinearAllocator{ kCpuWritable }
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

	m_dynamicViewDescriptorHeap.CleanupUsedHeaps(fenceValue);
	m_dynamicSamplerDescriptorHeap.CleanupUsedHeaps(fenceValue);
	m_cpuLinearAllocator.CleanupUsedPages(fenceValue);

	if (bWaitForCompletion)
	{
		deviceManager->WaitForFence(fenceValue);
	}

	return fenceValue;
}


void CommandContext12::TransitionResource(ColorBufferPtr& colorBuffer, ResourceState newState, bool bFlushImmediate)
{
	// TODO: Try this with GetPlatformObject()

	ColorBuffer* colorBuffer12 = (ColorBuffer*)colorBuffer.get();
	assert(colorBuffer12 != nullptr);

	ResourceState oldState = colorBuffer->GetUsageState();

	if (m_type == CommandListType::Compute)
	{
		assert(IsValidComputeResourceState(oldState));
		assert(IsValidComputeResourceState(newState));
	}

	ID3D12Resource* resource = colorBuffer12->GetResource();

	TransitionResource_Internal(resource, ResourceStateToDX12(oldState), ResourceStateToDX12(newState), bFlushImmediate);

	colorBuffer->SetUsageState(newState);
}


void CommandContext12::TransitionResource(DepthBufferPtr& depthBuffer, ResourceState newState, bool bFlushImmediate)
{
	// TODO: Try this with GetPlatformObject()

	DepthBuffer* depthBuffer12 = (DepthBuffer*)depthBuffer.get();
	assert(depthBuffer12 != nullptr);

	ResourceState oldState = depthBuffer->GetUsageState();

	if (m_type == CommandListType::Compute)
	{
		assert(IsValidComputeResourceState(oldState));
		assert(IsValidComputeResourceState(newState));
	}

	ID3D12Resource* resource = depthBuffer12->GetResource();

	TransitionResource_Internal(resource, ResourceStateToDX12(oldState), ResourceStateToDX12(newState), bFlushImmediate);

	depthBuffer->SetUsageState(newState);
}


void CommandContext12::TransitionResource(GpuBufferPtr& gpuBuffer, ResourceState newState, bool bFlushImmediate)
{
	// TODO: Try this with GetPlatformObject()

	GpuBuffer* gpuBuffer12 = (GpuBuffer*)gpuBuffer.get();

	ResourceState oldState = gpuBuffer->GetUsageState();

	if (m_type == CommandListType::Compute)
	{
		assert(IsValidComputeResourceState(oldState));
		assert(IsValidComputeResourceState(newState));
	}

	ID3D12Resource* resource = gpuBuffer12->GetResource();

	TransitionResource_Internal(resource, ResourceStateToDX12(oldState), ResourceStateToDX12(newState), bFlushImmediate);

	gpuBuffer->SetUsageState(newState);
}


void CommandContext12::TransitionResource(TexturePtr& texture, ResourceState newState, bool bFlushImmediate)
{
	// TODO: Try this with GetPlatformObject()

	Texture* texture12 = (Texture*)texture.Get();

	ResourceState oldState = texture->GetUsageState();

	if (m_type == CommandListType::Compute)
	{
		assert(IsValidComputeResourceState(oldState));
		assert(IsValidComputeResourceState(newState));
	}

	ID3D12Resource* resource = texture12->GetResource();

	TransitionResource_Internal(resource, ResourceStateToDX12(oldState), ResourceStateToDX12(newState), bFlushImmediate);

	texture->SetUsageState(newState);
}


void CommandContext12::FlushResourceBarriers()
{
	if (m_numBarriersToFlush > 0)
	{
		m_commandList->ResourceBarrier(m_numBarriersToFlush, m_resourceBarrierBuffer);
		m_numBarriersToFlush = 0;
	}
}


DynAlloc CommandContext12::ReserveUploadMemory(size_t sizeInBytes)
{
	return m_cpuLinearAllocator.Allocate(sizeInBytes);
}


void CommandContext12::ClearUAV(GpuBufferPtr& gpuBuffer)
{
	// TODO: We need to allocate a GPU descriptor, so need to implement dynamic descriptor heaps to do this.
}


void CommandContext12::ClearColor(ColorBufferPtr& colorBuffer)
{
	FlushResourceBarriers();

	// TODO: Try this with GetPlatformObject()

	ColorBuffer* colorBuffer12 = (ColorBuffer*)colorBuffer.get();
	assert(colorBuffer12 != nullptr);

	m_commandList->ClearRenderTargetView(colorBuffer12->GetRtvHandle(), colorBuffer->GetClearColor().GetPtr(), 0, nullptr);
}


void CommandContext12::ClearColor(ColorBufferPtr& colorBuffer, Color clearColor)
{
	FlushResourceBarriers();

	// TODO: Try this with GetPlatformObject()

	ColorBuffer* colorBuffer12 = (ColorBuffer*)colorBuffer.get();
	assert(colorBuffer12 != nullptr);

	m_commandList->ClearRenderTargetView(colorBuffer12->GetRtvHandle(), clearColor.GetPtr(), 0, nullptr);
}


void CommandContext12::ClearDepth(DepthBufferPtr& depthBuffer)
{
	FlushResourceBarriers();

	// TODO: Try this with GetPlatformObject()

	DepthBuffer* depthBuffer12 = (DepthBuffer*)depthBuffer.get();
	assert(depthBuffer12 != nullptr);

	m_commandList->ClearDepthStencilView(depthBuffer12->GetDsvHandle(DepthStencilAspect::ReadWrite), D3D12_CLEAR_FLAG_DEPTH, depthBuffer->GetClearDepth(), depthBuffer->GetClearStencil(), 0, nullptr);
}


void CommandContext12::ClearStencil(DepthBufferPtr& depthBuffer)
{
	FlushResourceBarriers();

	// TODO: Try this with GetPlatformObject()

	DepthBuffer* depthBuffer12 = (DepthBuffer*)depthBuffer.get();
	assert(depthBuffer12 != nullptr);

	m_commandList->ClearDepthStencilView(depthBuffer12->GetDsvHandle(DepthStencilAspect::ReadWrite), D3D12_CLEAR_FLAG_STENCIL, depthBuffer->GetClearDepth(), depthBuffer->GetClearStencil(), 0, nullptr);
}


void CommandContext12::ClearDepthAndStencil(DepthBufferPtr& depthBuffer)
{
	FlushResourceBarriers();

	// TODO: Try this with GetPlatformObject()

	DepthBuffer* depthBuffer12 = (DepthBuffer*)depthBuffer.get();
	assert(depthBuffer12 != nullptr);

	m_commandList->ClearDepthStencilView(depthBuffer12->GetDsvHandle(DepthStencilAspect::ReadWrite), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depthBuffer->GetClearDepth(), depthBuffer->GetClearStencil(), 0, nullptr);
}


void CommandContext12::BeginRendering(ColorBufferPtr& colorBuffer)
{
	assert(!m_isRendering);
	ResetRenderTargets();

	// TODO: Try this with GetPlatformObject()

	ColorBuffer* colorBuffer12 = (ColorBuffer*)colorBuffer.get();
	assert(colorBuffer12 != nullptr);

	m_rtvs[0] = colorBuffer12->GetRtvHandle();
	m_rtvFormats[0] = FormatToDxgi(colorBuffer->GetFormat()).rtvFormat;
	m_numRtvs = 1;

	BindRenderTargets();

	m_isRendering = true;
}


void CommandContext12::BeginRendering(ColorBufferPtr& colorBuffer, DepthBufferPtr& depthBuffer, DepthStencilAspect depthStencilAspect)
{
	assert(!m_isRendering);
	ResetRenderTargets();

	// TODO: Try this with GetPlatformObject()

	ColorBuffer* colorBuffer12 = (ColorBuffer*)colorBuffer.get();
	assert(colorBuffer12 != nullptr);

	DepthBuffer* depthBuffer12 = (DepthBuffer*)depthBuffer.get();
	assert(depthBuffer12 != nullptr);

	m_rtvs[0] = colorBuffer12->GetRtvHandle();
	m_rtvFormats[0] = FormatToDxgi(colorBuffer->GetFormat()).rtvFormat;
	m_numRtvs = 1;

	m_dsv = depthBuffer12->GetDsvHandle(depthStencilAspect);
	m_dsvFormat = FormatToDxgi(depthBuffer->GetFormat()).rtvFormat;
	m_hasDsv = true;

	BindRenderTargets();

	m_isRendering = true;
}


void CommandContext12::BeginRendering(DepthBufferPtr& depthBuffer, DepthStencilAspect depthStencilAspect)
{
	assert(!m_isRendering);
	ResetRenderTargets();

	// TODO: Try this with GetPlatformObject()

	DepthBuffer* depthBuffer12 = (DepthBuffer*)depthBuffer.get();
	assert(depthBuffer12 != nullptr);

	m_dsv = depthBuffer12->GetDsvHandle(depthStencilAspect);
	m_dsvFormat = FormatToDxgi(depthBuffer->GetFormat()).rtvFormat;
	m_hasDsv = true;

	BindRenderTargets();

	m_isRendering = true;
}


void CommandContext12::BeginRendering(std::span<ColorBufferPtr>& colorBuffers)
{
	assert(!m_isRendering);
	assert(colorBuffers.size() <= 8);

	ResetRenderTargets();

	// TODO: Try this with GetPlatformObject()

	uint32_t i = 0;
	for (const auto& colorBuffer : colorBuffers)
	{
		const ColorBuffer* colorBuffer12 = (ColorBuffer*)colorBuffer.get();
		assert(colorBuffer12 != nullptr);

		m_rtvs[i] = colorBuffer12->GetRtvHandle();
		m_rtvFormats[i] = FormatToDxgi(colorBuffer->GetFormat()).rtvFormat;
		++i;
	}
	m_numRtvs = (uint32_t)colorBuffers.size();

	BindRenderTargets();

	m_isRendering = true;
}


void CommandContext12::BeginRendering(std::span<ColorBufferPtr>& colorBuffers, DepthBufferPtr& depthBuffer, DepthStencilAspect depthStencilAspect)
{
	assert(!m_isRendering);
	assert(colorBuffers.size() <= 8);

	ResetRenderTargets();

	// TODO: Try this with GetPlatformObject()

	uint32_t i = 0;
	for (const auto& colorBuffer : colorBuffers)
	{
		const ColorBuffer* colorBuffer12 = (ColorBuffer*)colorBuffer.get();
		assert(colorBuffer12 != nullptr);

		m_rtvs[i] = colorBuffer12->GetRtvHandle();
		m_rtvFormats[i] = FormatToDxgi(colorBuffer->GetFormat()).rtvFormat;
		++i;
	}
	m_numRtvs = (uint32_t)colorBuffers.size();

	DepthBuffer* depthBuffer12 = (DepthBuffer*)depthBuffer.get();
	assert(depthBuffer12 != nullptr);

	m_dsv = depthBuffer12->GetDsvHandle(depthStencilAspect);
	m_dsvFormat = FormatToDxgi(depthBuffer->GetFormat()).rtvFormat;
	m_hasDsv = true;

	BindRenderTargets();

	m_isRendering = true;
}


void CommandContext12::EndRendering()
{
	assert(m_isRendering);
	m_isRendering = false;
}


void CommandContext12::SetRootSignature(RootSignaturePtr& rootSignature)
{
	assert(m_type == CommandListType::Direct || m_type == CommandListType::Compute);

	// TODO: Try this with GetPlatformObject()
	RootSignature* rootSignature12 = (RootSignature*)rootSignature.get();
	assert(rootSignature12 != nullptr);

	ID3D12RootSignature* d3d12RootSignature = rootSignature12->GetRootSignature();

	if (m_type == CommandListType::Direct)
	{
		m_computeRootSignature = nullptr;
		if (m_graphicsRootSignature == d3d12RootSignature)
		{
			return;
		}

		m_commandList->SetGraphicsRootSignature(d3d12RootSignature);
		m_graphicsRootSignature = d3d12RootSignature;
		m_dynamicViewDescriptorHeap.ParseGraphicsRootSignature(*rootSignature12);
		m_dynamicSamplerDescriptorHeap.ParseGraphicsRootSignature(*rootSignature12);
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
		m_dynamicViewDescriptorHeap.ParseComputeRootSignature(*rootSignature12);
		m_dynamicSamplerDescriptorHeap.ParseComputeRootSignature(*rootSignature12);
	}
}


void CommandContext12::SetGraphicsPipeline(GraphicsPipelineStatePtr& graphicsPipeline)
{
	// TODO: Try this with GetPlatformObject()
	GraphicsPipelineState* graphicsPipeline12 = (GraphicsPipelineState*)graphicsPipeline.get();
	assert(graphicsPipeline12 != nullptr);

	m_computePipelineState = nullptr;

	// TODO: Pass handle in as function parameter
	ID3D12PipelineState* graphicsPSO = graphicsPipeline12->GetPipelineState();

	if (m_graphicsPipelineState != graphicsPSO)
	{
		m_commandList->SetPipelineState(graphicsPSO);
		m_graphicsPipelineState = graphicsPSO;
	}

	SetPrimitiveTopology(graphicsPipeline->GetPrimitiveTopology());
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


void CommandContext12::SetConstantBuffer(uint32_t rootIndex, GpuBufferPtr& gpuBuffer)
{
	assert(m_type == CommandListType::Direct || m_type == CommandListType::Compute);

	// TODO: Try this with GetPlatformObject()
	GpuBuffer* gpuBuffer12 = (GpuBuffer*)gpuBuffer.get();
	assert(gpuBuffer12 != nullptr);

	uint64_t gpuAddress = gpuBuffer12->GetGpuAddress();

	if (m_type == CommandListType::Direct)
	{
		m_commandList->SetGraphicsRootConstantBufferView(rootIndex, gpuAddress);
	}
	else
	{
		m_commandList->SetComputeRootConstantBufferView(rootIndex, gpuAddress);
	}
}


void CommandContext12::SetDescriptors(uint32_t rootIndex, DescriptorSetPtr& descriptorSet)
{
	SetDescriptors_Internal(rootIndex, descriptorSet);
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
		SetDescriptors_Internal(i, resourceSet[i]);
	}
}


void CommandContext12::SetSRV(uint32_t rootIndex, uint32_t offset, ColorBufferPtr& colorBuffer)
{
	// TODO: Try this with GetPlatformObject()
	ColorBuffer* colorBuffer12 = (ColorBuffer*)colorBuffer.get();
	assert(colorBuffer12 != nullptr);

	auto descriptor = colorBuffer12->GetSrvHandle();

	SetDynamicDescriptors_Internal(rootIndex, offset, 1, &descriptor);
}


void CommandContext12::SetSRV(uint32_t rootIndex, uint32_t offset, DepthBufferPtr& depthBuffer, bool depthSrv)
{
	// TODO: Try this with GetPlatformObject()
	DepthBuffer* depthBuffer12 = (DepthBuffer*)depthBuffer.get();
	assert(depthBuffer12 != nullptr);

	auto descriptor = depthBuffer12->GetSrvHandle(depthSrv);

	SetDynamicDescriptors_Internal(rootIndex, offset, 1, &descriptor);
}


void CommandContext12::SetSRV(uint32_t rootIndex, uint32_t offset, GpuBufferPtr& gpuBuffer)
{
	// TODO: Try this with GetPlatformObject()
	GpuBuffer* gpuBuffer12 = (GpuBuffer*)gpuBuffer.get();
	assert(gpuBuffer12 != nullptr);

	auto descriptor = gpuBuffer12->GetSrvHandle();

	SetDynamicDescriptors_Internal(rootIndex, offset, 1, &descriptor);
}


void CommandContext12::SetSRV(uint32_t rootIndex, uint32_t offset, TexturePtr& texture)
{
	// TODO: Try this with GetPlatformObject()
	Texture* texture12 = (Texture*)texture.Get();
	assert(texture12 != nullptr);

	auto descriptor = texture12->GetSrvHandle();

	SetDynamicDescriptors_Internal(rootIndex, offset, 1, &descriptor);
}


void CommandContext12::SetUAV(uint32_t rootIndex, uint32_t offset, ColorBufferPtr& colorBuffer)
{
	// TODO: Try this with GetPlatformObject()
	ColorBuffer* colorBuffer12 = (ColorBuffer*)colorBuffer.get();
	assert(colorBuffer12 != nullptr);

	// TODO: Need a UAV index parameter
	auto descriptor = colorBuffer12->GetUavHandle(0);

	SetDynamicDescriptors_Internal(rootIndex, offset, 1, &descriptor);
}


void CommandContext12::SetUAV(uint32_t rootIndex, uint32_t offset, DepthBufferPtr& depthBuffer)
{
	// TODO: support this
	assert(false);
}


void CommandContext12::SetUAV(uint32_t rootIndex, uint32_t offset, GpuBufferPtr& gpuBuffer)
{
	// TODO: Try this with GetPlatformObject()
	GpuBuffer* gpuBuffer12 = (GpuBuffer*)gpuBuffer.get();
	assert(gpuBuffer12 != nullptr);

	auto descriptor = gpuBuffer12->GetUavHandle();

	SetDynamicDescriptors_Internal(rootIndex, offset, 1, &descriptor);
}


void CommandContext12::SetCBV(uint32_t rootIndex, uint32_t offset, GpuBufferPtr& gpuBuffer)
{
	// TODO: Try this with GetPlatformObject()
	GpuBuffer* gpuBuffer12 = (GpuBuffer*)gpuBuffer.get();
	assert(gpuBuffer12 != nullptr);

	auto descriptor = gpuBuffer12->GetCbvHandle();

	SetDynamicDescriptors_Internal(rootIndex, offset, 1, &descriptor);
}


void CommandContext12::SetIndexBuffer(GpuBufferPtr& gpuBuffer)
{
	// TODO: Try this with GetPlatformObject()
	GpuBuffer* gpuBuffer12 = (GpuBuffer*)gpuBuffer.get();
	assert(gpuBuffer12 != nullptr);

	const bool is16Bit = gpuBuffer->GetElementSize() == sizeof(uint16_t);
	D3D12_INDEX_BUFFER_VIEW ibv{
		.BufferLocation		= gpuBuffer12->GetGpuAddress(),
		.SizeInBytes		= (uint32_t)gpuBuffer->GetBufferSize(),
		.Format				= is16Bit ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT,
	};
	m_commandList->IASetIndexBuffer(&ibv);
}


void CommandContext12::SetVertexBuffer(uint32_t slot, GpuBufferPtr& gpuBuffer)
{
	// TODO: Try this with GetPlatformObject()
	GpuBuffer* gpuBuffer12 = (GpuBuffer*)gpuBuffer.get();
	assert(gpuBuffer12 != nullptr);

	D3D12_VERTEX_BUFFER_VIEW vbv{
		.BufferLocation		= gpuBuffer12->GetGpuAddress(),
		.SizeInBytes		= (uint32_t)gpuBuffer->GetBufferSize(),
		.StrideInBytes		= (uint32_t)gpuBuffer->GetElementSize()
	};
	m_commandList->IASetVertexBuffers(slot, 1, &vbv);
}


void CommandContext12::SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexStride, DynAlloc dynAlloc)
{
	const size_t bufferSize = numVertices * vertexStride;

	D3D12_VERTEX_BUFFER_VIEW vbv{
		.BufferLocation		= dynAlloc.gpuAddress,
		.SizeInBytes		= (uint32_t)bufferSize,
		.StrideInBytes		= (uint32_t)vertexStride
	};
	m_commandList->IASetVertexBuffers(slot, 1, &vbv);
}


void CommandContext12::SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexStride, const void* data)
{
	assert(data != nullptr && Math::IsAligned(data, 16));

	const size_t bufferSize = Math::AlignUp(numVertices * vertexStride, 16);

	DynAlloc dynAlloc = ReserveUploadMemory(bufferSize);

	SIMDMemCopy(dynAlloc.dataPtr, data, bufferSize >> 4);

	D3D12_VERTEX_BUFFER_VIEW vbv{
		.BufferLocation		= dynAlloc.gpuAddress,
		.SizeInBytes		= (uint32_t)bufferSize,
		.StrideInBytes		= (uint32_t)vertexStride
	};
	m_commandList->IASetVertexBuffers(slot, 1, &vbv);
}


void CommandContext12::SetDynamicIndexBuffer(uint32_t indexCount, bool indexSize16Bit, DynAlloc dynAlloc)
{
	const size_t elementSize = indexSize16Bit ? sizeof(uint16_t) : sizeof(uint32_t);

	D3D12_INDEX_BUFFER_VIEW ibv{
		.BufferLocation		= dynAlloc.gpuAddress,
		.SizeInBytes		= (uint32_t)(indexCount * elementSize),
		.Format				= indexSize16Bit ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT
	};
	m_commandList->IASetIndexBuffer(&ibv);
}


void CommandContext12::SetDynamicIndexBuffer(uint32_t indexCount, bool indexSize16Bit, const void* data)
{
	assert(data != nullptr && Math::IsAligned(data, 16));

	const size_t elementSize = indexSize16Bit ? sizeof(uint16_t) : sizeof(uint32_t);
	const size_t bufferSize = Math::AlignUp(indexCount * elementSize, 16);

	DynAlloc dynAlloc = ReserveUploadMemory(bufferSize);

	SIMDMemCopy(dynAlloc.dataPtr, data, bufferSize >> 4);

	D3D12_INDEX_BUFFER_VIEW ibv{
		.BufferLocation		= dynAlloc.gpuAddress,
		.SizeInBytes		= (uint32_t)(indexCount * elementSize),
		.Format				= indexSize16Bit ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT
	};
	m_commandList->IASetIndexBuffer(&ibv);
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


void CommandContext12::InitializeBuffer_Internal(GpuBufferPtr& destBuffer, const void* bufferData, size_t numBytes, size_t offset)
{ 
	// TODO: Try this with GetPlatformObject()
	GpuBuffer* destBuffer12 = (GpuBuffer*)destBuffer.get();
	assert(destBuffer12 != nullptr);

	// copy data to the intermediate upload heap and then schedule a copy from the upload heap to the destination buffer
	DynAlloc mem = ReserveUploadMemory(numBytes);
	SIMDMemCopy(mem.dataPtr, bufferData, Math::DivideByMultiple(numBytes, 16));
	ID3D12Resource* stagingBuffer = reinterpret_cast<ID3D12Resource*>(mem.resource);

	// Schedule a GPU data copy from the staging buffer to the destination buffer
	TransitionResource(destBuffer, ResourceState::CopyDest, true);
	m_commandList->CopyBufferRegion(destBuffer12->GetResource(), offset, stagingBuffer, 0, numBytes);
	TransitionResource(destBuffer, ResourceState::GenericRead, true);
}


void CommandContext12::InitializeTexture_Internal(TexturePtr& destTexture, const TextureInitializer& texInit)
{
	Texture* texture12 = (Texture*)destTexture.Get();
	assert(texture12 != nullptr);

	// Copy subresource info from Luna struct to D3D12 struct
	const uint32_t numSubresources = (uint32_t)texInit.subResourceData.size();
	std::vector<D3D12_SUBRESOURCE_DATA> subresources{ numSubresources };
	for (uint32_t i = 0; i < numSubresources; ++i)
	{
		subresources[i] = TextureSubResourceDataToDX12(texInit.subResourceData[i]);
	}

	// Copy data to the intermediate upload heap and then schedule a copy from the upload heap to the destination texture
	uint64_t uploadBufferSize = GetRequiredIntermediateSize(texture12->GetResource(), 0, numSubresources);
	DynAlloc mem = ReserveUploadMemory(uploadBufferSize);
	ID3D12Resource* stagingBuffer = reinterpret_cast<ID3D12Resource*>(mem.resource);

	UpdateSubresources(m_commandList, texture12->GetResource(), stagingBuffer, 0, 0, numSubresources, subresources.data());
	TransitionResource(destTexture, ResourceState::GenericRead, true);
}


void CommandContext12::SetDescriptors_Internal(uint32_t rootIndex, DescriptorSetPtr& descriptorSet)
{
	assert(m_type == CommandListType::Direct || m_type == CommandListType::Compute);

	// TODO: Try this with GetPlatformObject()
	DescriptorSet* descriptorSet12 = (DescriptorSet*)descriptorSet.get();
	assert(descriptorSet12 != nullptr);

	if (!descriptorSet12->HasBindableDescriptors())
	{
		return;
	}

	// Copy descriptors
	descriptorSet12->UpdateGpuDescriptors();

	auto gpuDescriptor = descriptorSet12->GetGpuDescriptorHandle();
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
	else if (descriptorSet12->GetGpuAddress() != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
	{
		const D3D12_GPU_VIRTUAL_ADDRESS gpuFinalAddress = descriptorSet12->GetGpuAddressWithOffset();
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