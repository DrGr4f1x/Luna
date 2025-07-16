//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#pragma once

#include "Graphics\Enums.h"
#include "Graphics\Texture.h"

namespace Luna
{

// Forward declarations
class IColorBuffer;
class CommandContext;
class ComputeContext;
class IDepthBuffer;
class IDescriptorSet;
class IGpuBuffer;
class GraphicsContext;
class IComputePipeline;
class IGraphicsPipeline;
class IQueryHeap;
class ResourceSet;
class IRootSignature;


using ColorBufferPtr = std::shared_ptr<IColorBuffer>;
using ComputePipelinePtr = std::shared_ptr<IComputePipeline>;
using DepthBufferPtr = std::shared_ptr<IDepthBuffer>;
using DescriptorSetPtr = std::shared_ptr<IDescriptorSet>;
using GpuBufferPtr = std::shared_ptr<IGpuBuffer>;
using GraphicsPipelinePtr = std::shared_ptr<IGraphicsPipeline>;
using QueryHeapPtr = std::shared_ptr<IQueryHeap>;
using RootSignaturePtr = std::shared_ptr<IRootSignature>;


class __declspec(uuid("ECBD0FFD-6571-4836-9DBB-7DC6436E086F")) ICommandContext : public IUnknown
{
	friend class CommandContext;

public:
	virtual ~ICommandContext() = default;

	virtual void SetId(const std::string& id) = 0;
	virtual CommandListType GetType() const = 0;

	// Debug events and markers
	virtual void BeginEvent(const std::string& label) = 0;
	virtual void EndEvent() = 0;
	virtual void SetMarker(const std::string& label) = 0;

	virtual void Reset() = 0;
	virtual void Initialize() = 0;

	virtual void BeginFrame() = 0;

	// Flush existing commands and release the current context
	virtual uint64_t Finish(bool bWaitForCompletion = false) = 0;

	virtual void TransitionResource(IColorBuffer* colorBuffer, ResourceState newState, bool bFlushImmediate = false) = 0;
	virtual void TransitionResource(IDepthBuffer* depthBuffer, ResourceState newState, bool bFlushImmediate = false) = 0;
	virtual void TransitionResource(IGpuBuffer* gpuBuffer, ResourceState newState, bool bFlushImmediate = false) = 0;
	virtual void TransitionResource(ITexture* texture, ResourceState newState, bool bFlushImmediate = false) = 0;
	virtual void InsertUAVBarrier(const IColorBuffer* colorBuffer, bool bFlushImmediate = false) = 0;
	virtual void InsertUAVBarrier(const IGpuBuffer* gpuBuffer, bool bFlushImmediate = false) = 0;
	virtual void FlushResourceBarriers() = 0;

	virtual DynAlloc ReserveUploadMemory(size_t sizeInBytes) = 0;

	// Graphics context
	virtual void ClearUAV(IGpuBuffer* gpuBuffer) = 0;
	// TODO: Figure out how to implement this for Vulkan
	//virtual void ClearUAV(ColorBufferPtr& colorBuffer) = 0;
	virtual void ClearColor(IColorBuffer* colorBuffer) = 0;
	virtual void ClearColor(IColorBuffer* colorBuffer, Color clearColor) = 0;
	virtual void ClearDepth(IDepthBuffer* depthBuffer) = 0;
	virtual void ClearStencil(IDepthBuffer* depthBuffer) = 0;
	virtual void ClearDepthAndStencil(IDepthBuffer* depthBuffer) = 0;

	virtual void BeginRendering(const IColorBuffer* renderTarget) = 0;
	virtual void BeginRendering(const IColorBuffer* renderTarget, const IDepthBuffer* depthTarget, DepthStencilAspect depthStencilAspect) = 0;
	virtual void BeginRendering(const IDepthBuffer* depthTarget, DepthStencilAspect depthStencilAspect) = 0;
	virtual void BeginRendering(std::span<const IColorBuffer*> renderTargets) = 0;
	virtual void BeginRendering(std::span<const IColorBuffer*> renderTargets, const IDepthBuffer* depthTarget, DepthStencilAspect depthStencilAspect) = 0;
	virtual void EndRendering() = 0;

	virtual void BeginOcclusionQuery(const IQueryHeap* queryHeap, uint32_t heapIndex) = 0;
	virtual void EndOcclusionQuery(const IQueryHeap* queryHeap, uint32_t heapIndex) = 0;
	virtual void ResolveOcclusionQueries(const IQueryHeap* queryHeap, uint32_t startIndex, uint32_t numQueries, const IGpuBuffer* destBuffer, uint64_t destBufferOffset) = 0;
	virtual void ResetOcclusionQueries(const IQueryHeap* queryHeap, uint32_t startIndex, uint32_t numQueries) = 0;

	virtual void SetRootSignature(CommandListType type, const IRootSignature* rootSignature) = 0;
	virtual void SetGraphicsPipeline(const IGraphicsPipeline* graphicsPipeline) = 0;
	virtual void SetComputePipeline(const IComputePipeline* computePipeline) = 0;

	virtual void SetViewport(float x, float y, float w, float h, float minDepth, float maxDepth) = 0;
	virtual void SetScissor(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom) = 0;
	// TODO: Support separate front and back stencil
	virtual void SetStencilRef(uint32_t stencilRef) = 0;
	virtual void SetBlendFactor(Color blendFactor) = 0;
	virtual void SetPrimitiveTopology(PrimitiveTopology topology) = 0;

	virtual void SetConstantArray(CommandListType type, uint32_t rootIndex, uint32_t numConstants, const void* constants, uint32_t offset) = 0;
	virtual void SetConstant(CommandListType type, uint32_t rootIndex, uint32_t offset, DWParam val) = 0;
	virtual void SetConstants(CommandListType type, uint32_t rootIndex, DWParam x) = 0;
	virtual void SetConstants(CommandListType type, uint32_t rootIndex, DWParam x, DWParam y) = 0;
	virtual void SetConstants(CommandListType type, uint32_t rootIndex, DWParam x, DWParam y, DWParam z) = 0;
	virtual void SetConstants(CommandListType type, uint32_t rootIndex, DWParam x, DWParam y, DWParam z, DWParam w) = 0;
	virtual void SetConstantBuffer(CommandListType type, uint32_t rootIndex, const IGpuBuffer* gpuBuffer) = 0;
	virtual void SetDescriptors(CommandListType type, uint32_t rootIndex, IDescriptorSet* descriptorSet) = 0;
	virtual void SetResources(CommandListType type, ResourceSet& resourceSet) = 0;

	virtual void SetSRV(CommandListType type, uint32_t rootIndex, uint32_t offset, ColorBufferPtr& colorBuffer) = 0;
	virtual void SetSRV(CommandListType type, uint32_t rootIndex, uint32_t offset, DepthBufferPtr& depthBuffer, bool depthSrv) = 0;
	virtual void SetSRV(CommandListType type, uint32_t rootIndex, uint32_t offset, GpuBufferPtr& gpuBuffer) = 0;
	virtual void SetSRV(CommandListType type, uint32_t rootIndex, uint32_t offset, TexturePtr& texture) = 0;

	virtual void SetUAV(CommandListType type, uint32_t rootIndex, uint32_t offset, ColorBufferPtr& colorBuffer) = 0;
	virtual void SetUAV(CommandListType type, uint32_t rootIndex, uint32_t offset, DepthBufferPtr& depthBuffer) = 0;
	virtual void SetUAV(CommandListType type, uint32_t rootIndex, uint32_t offset, GpuBufferPtr& gpuBuffer) = 0;

	virtual void SetCBV(CommandListType type, uint32_t rootIndex, uint32_t offset, GpuBufferPtr& gpuBuffer) = 0;

	virtual void SetIndexBuffer(const IGpuBuffer* gpuBuffer) = 0;
	virtual void SetVertexBuffer(uint32_t slot, const IGpuBuffer* gpuBuffer) = 0;
	virtual void SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexStride, DynAlloc dynAlloc) = 0;
	virtual void SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexStride, const void* data) = 0;
	virtual void SetDynamicIndexBuffer(uint32_t indexCount, bool indexSize16Bit, DynAlloc dynAlloc) = 0;
	virtual void SetDynamicIndexBuffer(uint32_t indexCount, bool indexSize16Bit, const void* data) = 0;

	virtual void DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount,
		uint32_t startVertexLocation, uint32_t startInstanceLocation) = 0;
	virtual void DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation,
		int32_t baseVertexLocation, uint32_t startInstanceLocation) = 0;

	virtual void Resolve(const IColorBuffer* srcBuffer, const IColorBuffer* destBuffer, Format format) = 0;

	// Compute context
	virtual void Dispatch(uint32_t groupCountX = 1, uint32_t groupCountY = 1, uint32_t groupCountZ = 1) = 0;
	virtual void Dispatch1D(uint32_t threadCountX, uint32_t groupSizeX = 64) = 0;
	virtual void Dispatch2D(uint32_t threadCountX, uint32_t threadCountY, uint32_t groupSizeX = 8, uint32_t groupSizeY = 8) = 0;
	virtual void Dispatch3D(uint32_t threadCountX, uint32_t threadCountY, uint32_t threadCountZ, uint32_t groupSizeX, uint32_t groupSizeY, uint32_t groupSizeZ) = 0;
	
protected:
	virtual void InitializeBuffer_Internal(IGpuBuffer* destBuffer, const void* bufferData, size_t numBytes, size_t offset) = 0;
	virtual void InitializeTexture_Internal(ITexture* destTexture, const TextureInitializer& texInit) = 0;
};



class CommandContext : NonCopyable
{
public:
	CommandContext(ICommandContext* pContextImpl)
		: m_contextImpl{ pContextImpl }
	{}

	static CommandContext& Begin(const std::string id = "");

	void SetId(const std::string& id);
	CommandListType GetType() const;

	// Debug events and markers
	void BeginEvent(const std::string& label);
	void EndEvent();
	void SetMarker(const std::string& label);

	void Reset();
	void Initialize();

	GraphicsContext& GetGraphicsContext() 
	{
		assert_msg(m_contextImpl->GetType() != CommandListType::Compute, "Cannot convert async compute context to graphics");
		return reinterpret_cast<GraphicsContext&>(*this);
	}

	ComputeContext& GetComputeContext() 
	{
		return reinterpret_cast<ComputeContext&>(*this);
	}

	static void InitializeBuffer(const GpuBufferPtr& destBuffer, const void* bufferData, size_t numBytes, size_t offset = 0);
	static void InitializeTexture(const TexturePtr& destTexture, const TextureInitializer& texInit);

	// Flush existing commands and release the current context
	uint64_t Finish(bool bWaitForCompletion = false);

	void TransitionResource(const ColorBufferPtr& colorBuffer, ResourceState newState, bool bFlushImmediate = false);
	void TransitionResource(const DepthBufferPtr& depthBuffer, ResourceState newState, bool bFlushImmediate = false);
	void TransitionResource(const GpuBufferPtr& gpuBuffer, ResourceState newState, bool bFlushImmediate = false);
	void TransitionResource(const TexturePtr& texture, ResourceState newState, bool bFlushImmediate = false);
	void InsertUAVBarrier(const ColorBufferPtr& colorBuffer, bool bFlushImmediate = false);
	void InsertUAVBarrier(const GpuBufferPtr& gpuBuffer, bool bFlushImmediate = false);
	void FlushResourceBarriers();

	DynAlloc ReserveUploadMemory(size_t sizeInBytes);

	void BeginFrame();

protected:
	wil::com_ptr<ICommandContext> m_contextImpl;
};


class GraphicsContext : public CommandContext
{
public:
	static GraphicsContext& Begin(const std::string id = "")
	{
		return CommandContext::Begin(id).GetGraphicsContext();
	}

	void ClearUAV(const GpuBufferPtr& gpuBuffer);
	//void ClearUAV(const ColorBufferPtr& colorBuffer);
	void ClearColor(const ColorBufferPtr& colorBuffer);
	void ClearColor(const ColorBufferPtr& colorBuffer, Color clearColor);
	void ClearDepth(const DepthBufferPtr& depthBuffer);
	void ClearStencil(const DepthBufferPtr& depthBuffer);
	void ClearDepthAndStencil(const DepthBufferPtr& depthBuffer);

	void BeginRendering(const ColorBufferPtr& renderTarget);
	void BeginRendering(const ColorBufferPtr& renderTarget, const DepthBufferPtr& depthTarget, DepthStencilAspect depthStencilAspect = DepthStencilAspect::ReadWrite);
	void BeginRendering(const DepthBufferPtr& depthTarget, DepthStencilAspect depthStencilAspect = DepthStencilAspect::ReadWrite);
	void BeginRendering(std::span<const ColorBufferPtr> renderTargets);
	void BeginRendering(std::span<const ColorBufferPtr> renderTargets, const DepthBufferPtr& depthTarget, DepthStencilAspect depthStencilAspect = DepthStencilAspect::ReadWrite);
	void EndRendering();

	void BeginOcclusionQuery(const QueryHeapPtr& queryHeap, uint32_t heapIndex);
	void EndOcclusionQuery(const QueryHeapPtr& queryHeap, uint32_t heapIndex);
	void ResolveOcclusionQueries(const QueryHeapPtr& queryHeap, uint32_t startIndex, uint32_t numQueries, const GpuBufferPtr& destBuffer, uint64_t destBufferOffset);
	void ResetOcclusionQueries(const QueryHeapPtr& queryHeap, uint32_t startIndex, uint32_t numQueries);

	void SetRootSignature(const RootSignaturePtr& rootSignature);
	void SetGraphicsPipeline(const GraphicsPipelinePtr& graphicsPipeline);

	void SetViewport(float x, float y, float w, float h, float minDepth = 0.0f, float maxDepth = 1.0f);
	void SetScissor(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom);
	void SetViewportAndScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
	void SetStencilRef(uint32_t stencilRef);
	void SetBlendFactor(Color blendFactor);
	void SetPrimitiveTopology(PrimitiveTopology topology);

	void SetConstantArray(uint32_t rootIndex, uint32_t numConstants, const void* constants);
	void SetConstantArray(uint32_t rootIndex, uint32_t numConstants, const void* constants, uint32_t offset);
	void SetConstant(uint32_t rootIndex, uint32_t offset, DWParam val);
	void SetConstants(uint32_t rootIndex, DWParam x);
	void SetConstants(uint32_t rootIndex, DWParam x, DWParam y);
	void SetConstants(uint32_t rootIndex, DWParam x, DWParam y, DWParam z);
	void SetConstants(uint32_t rootIndex, DWParam x, DWParam y, DWParam z, DWParam w);
	void SetConstantBuffer(uint32_t rootIndex, const GpuBufferPtr& gpuBuffer);
	void SetDescriptors(uint32_t rootIndex, const DescriptorSetPtr& descriptorSet);
	void SetResources(ResourceSet& resourceSet);

	void SetSRV(uint32_t rootIndex, uint32_t offset, ColorBufferPtr& colorBuffer);
	void SetSRV(uint32_t rootIndex, uint32_t offset, DepthBufferPtr& depthBuffer, bool depthSrv = true);
	void SetSRV(uint32_t rootIndex, uint32_t offset, GpuBufferPtr& gpuBuffer);
	void SetSRV(uint32_t rootIndex, uint32_t offset, TexturePtr& texture);

	void SetUAV(uint32_t rootIndex, uint32_t offset, ColorBufferPtr& colorBuffer);
	void SetUAV(uint32_t rootIndex, uint32_t offset, DepthBufferPtr& depthBuffer);
	void SetUAV(uint32_t rootIndex, uint32_t offset, GpuBufferPtr& gpuBuffer);

	void SetCBV(uint32_t rootIndex, uint32_t offset, GpuBufferPtr& gpuBuffer);

	void SetIndexBuffer(const GpuBufferPtr& gpuBuffer);
	void SetVertexBuffer(uint32_t slot, const GpuBufferPtr& gpuBuffer);
	void SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexStride, DynAlloc dynAlloc);
	void SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexStride, const void* data);
	void SetDynamicIndexBuffer(uint32_t indexCount, bool indexSize16Bit, DynAlloc dynAlloc);
	void SetDynamicIndexBuffer(uint32_t indexCount, bool indexSize16Bit, const void* data);

	void Draw(uint32_t vertexCount, uint32_t vertexStartOffset = 0);
	void DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation = 0, int32_t baseVertexLocation = 0);
	void DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount,
		uint32_t startVertexLocation = 0, uint32_t startInstanceLocation = 0);
	void DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation,
		int32_t baseVertexLocation, uint32_t startInstanceLocation);

	void Resolve(const ColorBufferPtr& srcBuffer, const ColorBufferPtr& destBuffer, Format format);
};


class ComputeContext : public CommandContext
{
public:
	static ComputeContext& Begin(const std::string id = "", bool bAsync = false);

	void SetRootSignature(const RootSignaturePtr& rootSignature);
	void SetComputePipeline(const ComputePipelinePtr& computePipeline);

	void SetConstantArray(uint32_t rootIndex, uint32_t numConstants, const void* constants);
	void SetConstantArray(uint32_t rootIndex, uint32_t numConstants, const void* constants, uint32_t offset);
	void SetConstant(uint32_t rootIndex, uint32_t offset, DWParam val);
	void SetConstants(uint32_t rootIndex, DWParam x);
	void SetConstants(uint32_t rootIndex, DWParam x, DWParam y);
	void SetConstants(uint32_t rootIndex, DWParam x, DWParam y, DWParam z);
	void SetConstants(uint32_t rootIndex, DWParam x, DWParam y, DWParam z, DWParam w);
	void SetConstantBuffer(uint32_t rootIndex, const GpuBufferPtr& gpuBuffer);
	void SetDescriptors(uint32_t rootIndex, const DescriptorSetPtr& descriptorSet);
	void SetResources(ResourceSet& resourceSet);

	void SetSRV(uint32_t rootIndex, uint32_t offset, ColorBufferPtr& colorBuffer);
	void SetSRV(uint32_t rootIndex, uint32_t offset, DepthBufferPtr& depthBuffer, bool depthSrv = true);
	void SetSRV(uint32_t rootIndex, uint32_t offset, GpuBufferPtr& gpuBuffer);
	void SetSRV(uint32_t rootIndex, uint32_t offset, TexturePtr& texture);

	void SetUAV(uint32_t rootIndex, uint32_t offset, ColorBufferPtr& colorBuffer);
	void SetUAV(uint32_t rootIndex, uint32_t offset, DepthBufferPtr& depthBuffer);
	void SetUAV(uint32_t rootIndex, uint32_t offset, GpuBufferPtr& gpuBuffer);

	void SetCBV(uint32_t rootIndex, uint32_t offset, GpuBufferPtr& gpuBuffer);

	void Dispatch(uint32_t groupCountX = 1, uint32_t groupCountY = 1, uint32_t groupCountZ = 1);
	void Dispatch1D(uint32_t threadCountX, uint32_t groupSizeX = 64);
	void Dispatch2D(uint32_t threadCountX, uint32_t threadCountY, uint32_t groupSizeX = 8, uint32_t groupSizeY = 8);
	void Dispatch3D(uint32_t threadCountX, uint32_t threadCountY, uint32_t threadCountZ, uint32_t groupSizeX, uint32_t groupSizeY, uint32_t groupSizeZ);
};


class ScopedDrawEvent
{
public:
	ScopedDrawEvent(CommandContext& context, const std::string& label)
		: m_context(context)
	{
		context.BeginEvent(label);
	}

	~ScopedDrawEvent()
	{
		m_context.EndEvent();
	}

private:
	CommandContext& m_context;
};


inline void CommandContext::SetId(const std::string& id)
{
	m_contextImpl->SetId(id);
}


inline CommandListType CommandContext::GetType() const
{
	return m_contextImpl->GetType();
}


inline void CommandContext::BeginEvent(const std::string& label)
{
	m_contextImpl->BeginEvent(label);
}


inline void CommandContext::EndEvent()
{
	m_contextImpl->EndEvent();
}


inline void CommandContext::SetMarker(const std::string& label)
{
	m_contextImpl->SetMarker(label);
}


inline void CommandContext::Reset()
{
	m_contextImpl->Reset();
}


inline void CommandContext::Initialize()
{
	m_contextImpl->Initialize();
}


inline void CommandContext::TransitionResource(const ColorBufferPtr& colorBuffer, ResourceState newState, bool bFlushImmediate)
{
	m_contextImpl->TransitionResource(colorBuffer.get(), newState, bFlushImmediate);
}


inline void CommandContext::TransitionResource(const DepthBufferPtr& depthBuffer, ResourceState newState, bool bFlushImmediate)
{
	m_contextImpl->TransitionResource(depthBuffer.get(), newState, bFlushImmediate);
}


inline void CommandContext::TransitionResource(const GpuBufferPtr& gpuBuffer, ResourceState newState, bool bFlushImmediate)
{
	m_contextImpl->TransitionResource(gpuBuffer.get(), newState, bFlushImmediate);
}


inline void CommandContext::TransitionResource(const TexturePtr& texture, ResourceState newState, bool bFlushImmediate)
{
	m_contextImpl->TransitionResource(texture.Get(), newState, bFlushImmediate);
}


inline void CommandContext::InsertUAVBarrier(const ColorBufferPtr& colorBuffer, bool bFlushImmediate)
{ 
	m_contextImpl->InsertUAVBarrier(colorBuffer.get(), bFlushImmediate);
}


inline void CommandContext::InsertUAVBarrier(const GpuBufferPtr& gpuBuffer, bool bFlushImmediate)
{
	m_contextImpl->InsertUAVBarrier(gpuBuffer.get(), bFlushImmediate);
}


inline void CommandContext::FlushResourceBarriers()
{
	m_contextImpl->FlushResourceBarriers();
}


inline DynAlloc CommandContext::ReserveUploadMemory(size_t sizeInBytes)
{
	return m_contextImpl->ReserveUploadMemory(sizeInBytes);
}


inline void CommandContext::BeginFrame()
{
	m_contextImpl->BeginFrame();
}


inline void GraphicsContext::ClearUAV(const GpuBufferPtr& gpuBuffer)
{
	m_contextImpl->ClearUAV(gpuBuffer.get());
}


//inline void GraphicsContext::ClearUAV(const ColorBufferPtr& colorBuffer)
//{
//	m_contextImpl->ClearUAV(colorBuffer.get());
//}


inline void GraphicsContext::ClearColor(const ColorBufferPtr& colorBuffer)
{
	m_contextImpl->ClearColor(colorBuffer.get());
}


inline void GraphicsContext::ClearColor(const ColorBufferPtr& colorBuffer, Color clearColor)
{
	m_contextImpl->ClearColor(colorBuffer.get(), clearColor);
}


inline void GraphicsContext::ClearDepth(const DepthBufferPtr& depthBuffer)
{
	m_contextImpl->ClearDepth(depthBuffer.get());
}


inline void GraphicsContext::ClearStencil(const DepthBufferPtr& depthBuffer)
{
	m_contextImpl->ClearStencil(depthBuffer.get());
}


inline void GraphicsContext::ClearDepthAndStencil(const DepthBufferPtr& depthBuffer)
{
	m_contextImpl->ClearDepthAndStencil(depthBuffer.get());
}


inline void GraphicsContext::BeginRendering(const ColorBufferPtr& renderTarget)
{
	m_contextImpl->BeginRendering(renderTarget.get());
}


inline void GraphicsContext::BeginRendering(const ColorBufferPtr& renderTarget, const DepthBufferPtr& depthTarget, DepthStencilAspect depthStencilAspect)
{
	m_contextImpl->BeginRendering(renderTarget.get(), depthTarget.get(), depthStencilAspect);
}


inline void GraphicsContext::BeginRendering(const DepthBufferPtr& depthTarget, DepthStencilAspect depthStencilAspect)
{
	m_contextImpl->BeginRendering(depthTarget.get(), depthStencilAspect);
}


inline void GraphicsContext::BeginRendering(std::span<const ColorBufferPtr> renderTargets)
{
	std::vector<const IColorBuffer*> renderTargetPtrs{ 8 };
	for (const auto& rt : renderTargets)
	{
		renderTargetPtrs.push_back(rt.get());
	}

	m_contextImpl->BeginRendering(renderTargetPtrs);
}


inline void GraphicsContext::BeginRendering(std::span<const ColorBufferPtr> renderTargets, const DepthBufferPtr& depthTarget, DepthStencilAspect depthStencilAspect)
{
	std::vector<const IColorBuffer*> renderTargetPtrs{ 8 };
	for (const auto& rt : renderTargets)
	{
		renderTargetPtrs.push_back(rt.get());
	}

	m_contextImpl->BeginRendering(renderTargetPtrs, depthTarget.get(), depthStencilAspect);
}


inline void GraphicsContext::EndRendering()
{
	m_contextImpl->EndRendering();
}


inline void GraphicsContext::BeginOcclusionQuery(const QueryHeapPtr& queryHeap, uint32_t heapIndex)
{
	m_contextImpl->BeginOcclusionQuery(queryHeap.get(), heapIndex);
}


inline void GraphicsContext::EndOcclusionQuery(const QueryHeapPtr& queryHeap, uint32_t heapIndex)
{
	m_contextImpl->EndOcclusionQuery(queryHeap.get(), heapIndex);
}


inline void GraphicsContext::ResolveOcclusionQueries(const QueryHeapPtr& queryHeap, uint32_t startIndex, uint32_t numQueries, const GpuBufferPtr& destBuffer, uint64_t destBufferOffset)
{
	m_contextImpl->ResolveOcclusionQueries(queryHeap.get(), startIndex, numQueries, destBuffer.get(), destBufferOffset);
}


inline void GraphicsContext::ResetOcclusionQueries(const QueryHeapPtr& queryHeap, uint32_t startIndex, uint32_t numQueries)
{
	m_contextImpl->ResetOcclusionQueries(queryHeap.get(), startIndex, numQueries);
}


inline void GraphicsContext::SetRootSignature(const RootSignaturePtr& rootSignature)
{
	m_contextImpl->SetRootSignature(CommandListType::Direct, rootSignature.get());
}


inline void GraphicsContext::SetGraphicsPipeline(const GraphicsPipelinePtr& graphicsPipeline)
{
	m_contextImpl->SetGraphicsPipeline(graphicsPipeline.get());
}


inline void GraphicsContext::SetViewport(float x, float y, float w, float h, float minDepth, float maxDepth)
{
	m_contextImpl->SetViewport(x, y, w, h, minDepth, maxDepth);
}


inline void GraphicsContext::SetScissor(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom)
{
	m_contextImpl->SetScissor(left, top, right, bottom);
}


inline void GraphicsContext::SetViewportAndScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
	m_contextImpl->SetViewport((float)x, (float)y, (float)w, (float)h, 0.0f, 1.0f);
	m_contextImpl->SetScissor(x, y, x + w, y + h);
}


inline void GraphicsContext::SetStencilRef(uint32_t stencilRef)
{
	m_contextImpl->SetStencilRef(stencilRef);
}


inline void GraphicsContext::SetBlendFactor(Color blendFactor)
{
	m_contextImpl->SetBlendFactor(blendFactor);
}


inline void GraphicsContext::SetPrimitiveTopology(PrimitiveTopology topology)
{
	m_contextImpl->SetPrimitiveTopology(topology);
}


inline void GraphicsContext::SetConstantArray(uint32_t rootIndex, uint32_t numConstants, const void* constants)
{
	m_contextImpl->SetConstantArray(CommandListType::Direct, rootIndex, numConstants, constants, 0);
}


inline void GraphicsContext::SetConstantArray(uint32_t rootIndex, uint32_t numConstants, const void* constants, uint32_t offset)
{
	m_contextImpl->SetConstantArray(CommandListType::Direct, rootIndex, numConstants, constants, offset);
}


inline void GraphicsContext::SetConstant(uint32_t rootIndex, uint32_t offset, DWParam val)
{
	m_contextImpl->SetConstant(CommandListType::Direct, rootIndex, offset, val);
}


inline void GraphicsContext::SetConstants(uint32_t rootIndex, DWParam x)
{
	m_contextImpl->SetConstants(CommandListType::Direct, rootIndex, x);
}


inline void GraphicsContext::SetConstants(uint32_t rootIndex, DWParam x, DWParam y)
{
	m_contextImpl->SetConstants(CommandListType::Direct, rootIndex, x, y);
}


inline void GraphicsContext::SetConstants(uint32_t rootIndex, DWParam x, DWParam y, DWParam z)
{
	m_contextImpl->SetConstants(CommandListType::Direct, rootIndex, x, y, z);
}


inline void GraphicsContext::SetConstants(uint32_t rootIndex, DWParam x, DWParam y, DWParam z, DWParam w)
{
	m_contextImpl->SetConstants(CommandListType::Direct, rootIndex, x, y, z, w);
}


inline void GraphicsContext::SetConstantBuffer(uint32_t rootIndex, const GpuBufferPtr& gpuBuffer)
{
	m_contextImpl->SetConstantBuffer(CommandListType::Direct, rootIndex, gpuBuffer.get());
}


inline void GraphicsContext::SetDescriptors(uint32_t rootIndex, const DescriptorSetPtr& descriptorSet)
{
	m_contextImpl->SetDescriptors(CommandListType::Direct, rootIndex, descriptorSet.get());
}


inline void GraphicsContext::SetResources(ResourceSet& resourceSet)
{
	m_contextImpl->SetResources(CommandListType::Direct, resourceSet);
}


inline void GraphicsContext::SetSRV(uint32_t rootParam, uint32_t offset, ColorBufferPtr& colorBuffer)
{
	m_contextImpl->SetSRV(CommandListType::Direct, rootParam, offset, colorBuffer);
}


inline void GraphicsContext::SetSRV(uint32_t rootParam, uint32_t offset, DepthBufferPtr& depthBuffer, bool depthSrv)
{
	m_contextImpl->SetSRV(CommandListType::Direct, rootParam, offset, depthBuffer, depthSrv);
}


inline void GraphicsContext::SetSRV(uint32_t rootParam, uint32_t offset, GpuBufferPtr& gpuBuffer)
{
	m_contextImpl->SetSRV(CommandListType::Direct, rootParam, offset, gpuBuffer);
}


inline void GraphicsContext::SetSRV(uint32_t rootParam, uint32_t offset, TexturePtr& texture)
{
	m_contextImpl->SetSRV(CommandListType::Direct, rootParam, offset, texture);
}


inline void GraphicsContext::SetUAV(uint32_t rootParam, uint32_t offset, ColorBufferPtr& colorBuffer)
{
	m_contextImpl->SetUAV(CommandListType::Direct, rootParam, offset, colorBuffer);
}


inline void GraphicsContext::SetUAV(uint32_t rootParam, uint32_t offset, DepthBufferPtr& depthBuffer)
{
	m_contextImpl->SetUAV(CommandListType::Direct, rootParam, offset, depthBuffer);
}


inline void GraphicsContext::SetUAV(uint32_t rootParam, uint32_t offset, GpuBufferPtr& gpuBuffer)
{
	m_contextImpl->SetUAV(CommandListType::Direct, rootParam, offset, gpuBuffer);
}


inline void GraphicsContext::SetCBV(uint32_t rootParam, uint32_t offset, GpuBufferPtr& gpuBuffer)
{
	m_contextImpl->SetCBV(CommandListType::Direct, rootParam, offset, gpuBuffer);
}


inline void GraphicsContext::SetIndexBuffer(const GpuBufferPtr& gpuBuffer)
{
	m_contextImpl->SetIndexBuffer(gpuBuffer.get());
}


inline void GraphicsContext::SetVertexBuffer(uint32_t slot, const GpuBufferPtr& gpuBuffer)
{
	m_contextImpl->SetVertexBuffer(slot, gpuBuffer.get());
}


inline void GraphicsContext::SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexStride, DynAlloc dynAlloc)
{
	m_contextImpl->SetDynamicVertexBuffer(slot, numVertices, vertexStride, dynAlloc);
}


inline void GraphicsContext::SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexStride, const void* data)
{
	m_contextImpl->SetDynamicVertexBuffer(slot, numVertices, vertexStride, data);
}


inline void GraphicsContext::SetDynamicIndexBuffer(uint32_t indexCount, bool indexSize16Bit, DynAlloc dynAlloc)
{
	m_contextImpl->SetDynamicIndexBuffer(indexCount, indexSize16Bit, dynAlloc);
}


inline void GraphicsContext::SetDynamicIndexBuffer(uint32_t indexCount, bool indexSize16Bit, const void* data)
{
	m_contextImpl->SetDynamicIndexBuffer(indexCount, indexSize16Bit, data);
}


inline void GraphicsContext::Draw(uint32_t vertexCount, uint32_t vertexStartOffset)
{
	m_contextImpl->DrawInstanced(vertexCount, 1, vertexStartOffset, 0);
}


inline void GraphicsContext::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation)
{
	m_contextImpl->DrawIndexedInstanced(indexCount, 1, startIndexLocation, baseVertexLocation, 0);
}


inline void GraphicsContext::DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount,
	uint32_t startVertexLocation, uint32_t startInstanceLocation)
{
	m_contextImpl->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}


inline void GraphicsContext::DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation,
	int32_t baseVertexLocation, uint32_t startInstanceLocation)
{
	m_contextImpl->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}


inline void GraphicsContext::Resolve(const ColorBufferPtr& srcBuffer, const ColorBufferPtr& destBuffer, Format format)
{
	m_contextImpl->Resolve(srcBuffer.get(), destBuffer.get(), format);
}


inline void ComputeContext::SetRootSignature(const RootSignaturePtr& rootSignature)
{
	m_contextImpl->SetRootSignature(CommandListType::Compute, rootSignature.get());
}


inline void ComputeContext::SetComputePipeline(const ComputePipelinePtr& computePipeline)
{
	m_contextImpl->SetComputePipeline(computePipeline.get());
}


inline void ComputeContext::SetConstantArray(uint32_t rootIndex, uint32_t numConstants, const void* constants)
{
	m_contextImpl->SetConstantArray(CommandListType::Compute, rootIndex, numConstants, constants, 0);
}


inline void ComputeContext::SetConstantArray(uint32_t rootIndex, uint32_t numConstants, const void* constants, uint32_t offset)
{
	m_contextImpl->SetConstantArray(CommandListType::Compute, rootIndex, numConstants, constants, offset);
}


inline void ComputeContext::SetConstant(uint32_t rootIndex, uint32_t offset, DWParam val)
{
	m_contextImpl->SetConstant(CommandListType::Compute, rootIndex, offset, val);
}


inline void ComputeContext::SetConstants(uint32_t rootIndex, DWParam x)
{
	m_contextImpl->SetConstants(CommandListType::Compute, rootIndex, x);
}


inline void ComputeContext::SetConstants(uint32_t rootIndex, DWParam x, DWParam y)
{
	m_contextImpl->SetConstants(CommandListType::Compute, rootIndex, x, y);
}


inline void ComputeContext::SetConstants(uint32_t rootIndex, DWParam x, DWParam y, DWParam z)
{
	m_contextImpl->SetConstants(CommandListType::Compute, rootIndex, x, y, z);
}


inline void ComputeContext::SetConstants(uint32_t rootIndex, DWParam x, DWParam y, DWParam z, DWParam w)
{
	m_contextImpl->SetConstants(CommandListType::Compute, rootIndex, x, y, z, w);
}


inline void ComputeContext::SetConstantBuffer(uint32_t rootIndex, const GpuBufferPtr& gpuBuffer)
{
	m_contextImpl->SetConstantBuffer(CommandListType::Compute, rootIndex, gpuBuffer.get());
}


inline void ComputeContext::SetDescriptors(uint32_t rootIndex, const DescriptorSetPtr& descriptorSet)
{
	m_contextImpl->SetDescriptors(CommandListType::Compute, rootIndex, descriptorSet.get());
}


inline void ComputeContext::SetResources(ResourceSet& resourceSet)
{
	m_contextImpl->SetResources(CommandListType::Compute, resourceSet);
}


inline void ComputeContext::SetSRV(uint32_t rootParam, uint32_t offset, ColorBufferPtr& colorBuffer)
{
	m_contextImpl->SetSRV(CommandListType::Compute, rootParam, offset, colorBuffer);
}


inline void ComputeContext::SetSRV(uint32_t rootParam, uint32_t offset, DepthBufferPtr& depthBuffer, bool depthSrv)
{
	m_contextImpl->SetSRV(CommandListType::Compute, rootParam, offset, depthBuffer, depthSrv);
}


inline void ComputeContext::SetSRV(uint32_t rootParam, uint32_t offset, GpuBufferPtr& gpuBuffer)
{
	m_contextImpl->SetSRV(CommandListType::Compute, rootParam, offset, gpuBuffer);
}


inline void ComputeContext::SetSRV(uint32_t rootParam, uint32_t offset, TexturePtr& texture)
{
	m_contextImpl->SetSRV(CommandListType::Compute, rootParam, offset, texture);
}


inline void ComputeContext::SetUAV(uint32_t rootParam, uint32_t offset, ColorBufferPtr& colorBuffer)
{
	m_contextImpl->SetUAV(CommandListType::Compute, rootParam, offset, colorBuffer);
}


inline void ComputeContext::SetUAV(uint32_t rootParam, uint32_t offset, DepthBufferPtr& depthBuffer)
{
	m_contextImpl->SetUAV(CommandListType::Compute, rootParam, offset, depthBuffer);
}


inline void ComputeContext::SetUAV(uint32_t rootParam, uint32_t offset, GpuBufferPtr& gpuBuffer)
{
	m_contextImpl->SetUAV(CommandListType::Compute, rootParam, offset, gpuBuffer);
}


inline void ComputeContext::SetCBV(uint32_t rootParam, uint32_t offset, GpuBufferPtr& gpuBuffer)
{
	m_contextImpl->SetCBV(CommandListType::Compute, rootParam, offset, gpuBuffer);
}


inline void ComputeContext::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
	m_contextImpl->Dispatch(groupCountX, groupCountY, groupCountZ);
}


inline void ComputeContext::Dispatch1D(uint32_t threadCountX, uint32_t groupSizeX)
{
	m_contextImpl->Dispatch1D(threadCountX, groupSizeX);
}


inline void ComputeContext::Dispatch2D(uint32_t threadCountX, uint32_t threadCountY, uint32_t groupSizeX, uint32_t groupSizeY)
{
	m_contextImpl->Dispatch2D(threadCountX, threadCountY, groupSizeX, groupSizeY);
}


inline void ComputeContext::Dispatch3D(uint32_t threadCountX, uint32_t threadCountY, uint32_t threadCountZ, uint32_t groupSizeX, uint32_t groupSizeY, uint32_t groupSizeZ)
{
	m_contextImpl->Dispatch3D(threadCountX, threadCountY, threadCountZ, groupSizeX, groupSizeY, groupSizeZ);
}

} // namespace Luna
