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
class IGraphicsPipelineState;
class ResourceSet;
class IRootSignature;


using ColorBufferPtr = std::shared_ptr<IColorBuffer>;
using DepthBufferPtr = std::shared_ptr<IDepthBuffer>;
using DescriptorSetPtr = std::shared_ptr<IDescriptorSet>;
using GpuBufferPtr = std::shared_ptr<IGpuBuffer>;
using GraphicsPipelineStatePtr = std::shared_ptr<IGraphicsPipelineState>;
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

	virtual void TransitionResource(ColorBufferPtr colorBuffer, ResourceState newState, bool bFlushImmediate = false) = 0;
	virtual void TransitionResource(DepthBufferPtr depthBuffer, ResourceState newState, bool bFlushImmediate = false) = 0;
	virtual void TransitionResource(GpuBufferPtr gpuBuffer, ResourceState newState, bool bFlushImmediate = false) = 0;
	virtual void TransitionResource(TexturePtr texture, ResourceState newState, bool bFlushImmediate = false) = 0;
	virtual void FlushResourceBarriers() = 0;

	virtual DynAlloc ReserveUploadMemory(size_t sizeInBytes) = 0;

	// Graphics context
	virtual void ClearUAV(GpuBufferPtr gpuBuffer) = 0;
	// TODO: Figure out how to implement this for Vulkan
	//virtual void ClearUAV(ColorBufferPtr colorBuffer) = 0;
	virtual void ClearColor(ColorBufferPtr colorBuffer) = 0;
	virtual void ClearColor(ColorBufferPtr colorBuffer, Color clearColor) = 0;
	virtual void ClearDepth(DepthBufferPtr depthBuffer) = 0;
	virtual void ClearStencil(DepthBufferPtr depthBuffer) = 0;
	virtual void ClearDepthAndStencil(DepthBufferPtr depthBuffer) = 0;

	virtual void BeginRendering(ColorBufferPtr renderTarget) = 0;
	virtual void BeginRendering(ColorBufferPtr renderTarget, DepthBufferPtr depthTarget, DepthStencilAspect depthStencilAspect) = 0;
	virtual void BeginRendering(DepthBufferPtr depthTarget, DepthStencilAspect depthStencilAspect) = 0;
	virtual void BeginRendering(std::span<ColorBufferPtr> renderTargets) = 0;
	virtual void BeginRendering(std::span<ColorBufferPtr> renderTargets, DepthBufferPtr depthTarget, DepthStencilAspect depthStencilAspect) = 0;
	virtual void EndRendering() = 0;

	virtual void SetRootSignature(RootSignaturePtr rootSignature) = 0;
	virtual void SetGraphicsPipeline(GraphicsPipelineStatePtr graphicsPipeline) = 0;

	virtual void SetViewport(float x, float y, float w, float h, float minDepth, float maxDepth) = 0;
	virtual void SetScissor(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom) = 0;
	// TODO: Support separate front and back stencil
	virtual void SetStencilRef(uint32_t stencilRef) = 0;
	virtual void SetBlendFactor(Color blendFactor) = 0;
	virtual void SetPrimitiveTopology(PrimitiveTopology topology) = 0;

	virtual void SetConstantArray(uint32_t rootIndex, uint32_t numConstants, const void* constants, uint32_t offset) = 0;
	virtual void SetConstant(uint32_t rootIndex, uint32_t offset, DWParam val) = 0;
	virtual void SetConstants(uint32_t rootIndex, DWParam x) = 0;
	virtual void SetConstants(uint32_t rootIndex, DWParam x, DWParam y) = 0;
	virtual void SetConstants(uint32_t rootIndex, DWParam x, DWParam y, DWParam z) = 0;
	virtual void SetConstants(uint32_t rootIndex, DWParam x, DWParam y, DWParam z, DWParam w) = 0;
	virtual void SetConstantBuffer(uint32_t rootIndex, GpuBufferPtr gpuBuffer) = 0;
	virtual void SetDescriptors(uint32_t rootIndex, DescriptorSetPtr descriptorSet) = 0;
	virtual void SetResources(ResourceSet& resourceSet) = 0;

	virtual void SetSRV(uint32_t rootIndex, uint32_t offset, ColorBufferPtr colorBuffer) = 0;
	virtual void SetSRV(uint32_t rootIndex, uint32_t offset, DepthBufferPtr depthBuffer, bool depthSrv) = 0;
	virtual void SetSRV(uint32_t rootIndex, uint32_t offset, GpuBufferPtr gpuBuffer) = 0;
	virtual void SetSRV(uint32_t rootIndex, uint32_t offset, TexturePtr texture) = 0;

	virtual void SetUAV(uint32_t rootIndex, uint32_t offset, ColorBufferPtr colorBuffer) = 0;
	virtual void SetUAV(uint32_t rootIndex, uint32_t offset, DepthBufferPtr depthBuffer) = 0;
	virtual void SetUAV(uint32_t rootIndex, uint32_t offset, GpuBufferPtr gpuBuffer) = 0;

	virtual void SetCBV(uint32_t rootIndex, uint32_t offset, GpuBufferPtr gpuBuffer) = 0;

	virtual void SetIndexBuffer(GpuBufferPtr gpuBuffer) = 0;
	virtual void SetVertexBuffer(uint32_t slot, GpuBufferPtr gpuBuffer) = 0;
	virtual void SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexStride, DynAlloc dynAlloc) = 0;
	virtual void SetDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexStride, const void* data) = 0;
	virtual void SetDynamicIndexBuffer(uint32_t indexCount, bool indexSize16Bit, DynAlloc dynAlloc) = 0;
	virtual void SetDynamicIndexBuffer(uint32_t indexCount, bool indexSize16Bit, const void* data) = 0;

	virtual void DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount,
		uint32_t startVertexLocation, uint32_t startInstanceLocation) = 0;
	virtual void DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation,
		int32_t baseVertexLocation, uint32_t startInstanceLocation) = 0;

	// Compute context
	
protected:
	virtual void InitializeBuffer_Internal(GpuBufferPtr destBuffer, const void* bufferData, size_t numBytes, size_t offset) = 0;
	virtual void InitializeTexture_Internal(TexturePtr destTexture, const TextureInitializer& texInit) = 0;
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

	static void InitializeBuffer(GpuBufferPtr destBuffer, const void* bufferData, size_t numBytes, size_t offset = 0);
	static void InitializeTexture(TexturePtr destTexture, const TextureInitializer& texInit);

	// Flush existing commands and release the current context
	uint64_t Finish(bool bWaitForCompletion = false);

	void TransitionResource(ColorBufferPtr colorBuffer, ResourceState newState, bool bFlushImmediate = false);
	void TransitionResource(DepthBufferPtr depthBuffer, ResourceState newState, bool bFlushImmediate = false);
	void TransitionResource(GpuBufferPtr gpuBuffer, ResourceState newState, bool bFlushImmediate = false);
	void TransitionResource(TexturePtr texture, ResourceState newState, bool bFlushImmediate = false);
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

	void ClearUAV(GpuBufferPtr gpuBuffer);
	//void ClearUAV(ColorBufferPtr colorBuffer);
	void ClearColor(ColorBufferPtr colorBuffer);
	void ClearColor(ColorBufferPtr colorBuffer, Color clearColor);
	void ClearDepth(DepthBufferPtr depthBuffer);
	void ClearStencil(DepthBufferPtr depthBuffer);
	void ClearDepthAndStencil(DepthBufferPtr depthBuffer);

	void BeginRendering(ColorBufferPtr renderTarget);
	void BeginRendering(ColorBufferPtr renderTarget, DepthBufferPtr depthTarget, DepthStencilAspect depthStencilAspect = DepthStencilAspect::ReadWrite);
	void BeginRendering(DepthBufferPtr depthTarget, DepthStencilAspect depthStencilAspect = DepthStencilAspect::ReadWrite);
	void BeginRendering(std::span<ColorBufferPtr> renderTargets);
	void BeginRendering(std::span<ColorBufferPtr> renderTargets, DepthBufferPtr depthTarget, DepthStencilAspect depthStencilAspect = DepthStencilAspect::ReadWrite);
	void EndRendering();

	void SetRootSignature(RootSignaturePtr rootSignature);
	void SetGraphicsPipeline(GraphicsPipelineStatePtr graphicsPipeline);

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
	void SetConstantBuffer(uint32_t rootIndex, GpuBufferPtr gpuBuffer);
	void SetDescriptors(uint32_t rootIndex, DescriptorSetPtr descriptorSet);
	void SetResources(ResourceSet& resourceSet);

	void SetSRV(uint32_t rootIndex, uint32_t offset, ColorBufferPtr colorBuffer);
	void SetSRV(uint32_t rootIndex, uint32_t offset, DepthBufferPtr depthBuffer, bool depthSrv = true);
	void SetSRV(uint32_t rootIndex, uint32_t offset, GpuBufferPtr gpuBuffer);
	void SetSRV(uint32_t rootIndex, uint32_t offset, TexturePtr texture);

	void SetUAV(uint32_t rootIndex, uint32_t offset, ColorBufferPtr colorBuffer);
	void SetUAV(uint32_t rootIndex, uint32_t offset, DepthBufferPtr depthBuffer);
	void SetUAV(uint32_t rootIndex, uint32_t offset, GpuBufferPtr gpuBuffer);

	void SetCBV(uint32_t rootIndex, uint32_t offset, GpuBufferPtr gpuBuffer);

	void SetIndexBuffer(GpuBufferPtr gpuBuffer);
	void SetVertexBuffer(uint32_t slot, GpuBufferPtr gpuBuffer);
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
};


class ComputeContext : public CommandContext
{
public:
	static ComputeContext& Begin(const std::string id = "", bool bAsync = false);
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


inline void CommandContext::TransitionResource(ColorBufferPtr colorBuffer, ResourceState newState, bool bFlushImmediate)
{
	m_contextImpl->TransitionResource(colorBuffer, newState, bFlushImmediate);
}


inline void CommandContext::TransitionResource(DepthBufferPtr depthBuffer, ResourceState newState, bool bFlushImmediate)
{
	m_contextImpl->TransitionResource(depthBuffer, newState, bFlushImmediate);
}


inline void CommandContext::TransitionResource(GpuBufferPtr gpuBuffer, ResourceState newState, bool bFlushImmediate)
{
	m_contextImpl->TransitionResource(gpuBuffer, newState, bFlushImmediate);
}


inline void CommandContext::TransitionResource(TexturePtr texture, ResourceState newState, bool bFlushImmediate)
{
	m_contextImpl->TransitionResource(texture, newState, bFlushImmediate);
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


inline void GraphicsContext::ClearUAV(GpuBufferPtr gpuBuffer)
{
	m_contextImpl->ClearUAV(gpuBuffer);
}


//inline void GraphicsContext::ClearUAV(ColorBufferPtr colorBuffer)
//{
//	m_contextImpl->ClearUAV(colorBuffer);
//}


inline void GraphicsContext::ClearColor(ColorBufferPtr colorBuffer)
{
	m_contextImpl->ClearColor(colorBuffer);
}


inline void GraphicsContext::ClearColor(ColorBufferPtr colorBuffer, Color clearColor)
{
	m_contextImpl->ClearColor(colorBuffer, clearColor);
}


inline void GraphicsContext::ClearDepth(DepthBufferPtr depthBuffer)
{
	m_contextImpl->ClearDepth(depthBuffer);
}


inline void GraphicsContext::ClearStencil(DepthBufferPtr depthBuffer)
{
	m_contextImpl->ClearStencil(depthBuffer);
}


inline void GraphicsContext::ClearDepthAndStencil(DepthBufferPtr depthBuffer)
{
	m_contextImpl->ClearDepthAndStencil(depthBuffer);
}


inline void GraphicsContext::BeginRendering(ColorBufferPtr renderTarget)
{
	m_contextImpl->BeginRendering(renderTarget);
}


inline void GraphicsContext::BeginRendering(ColorBufferPtr renderTarget, DepthBufferPtr depthTarget, DepthStencilAspect depthStencilAspect)
{
	m_contextImpl->BeginRendering(renderTarget, depthTarget, depthStencilAspect);
}


inline void GraphicsContext::BeginRendering(DepthBufferPtr depthTarget, DepthStencilAspect depthStencilAspect)
{
	m_contextImpl->BeginRendering(depthTarget, depthStencilAspect);
}


inline void GraphicsContext::BeginRendering(std::span<ColorBufferPtr> renderTargets)
{
	m_contextImpl->BeginRendering(renderTargets);
}


inline void GraphicsContext::BeginRendering(std::span<ColorBufferPtr> renderTargets, DepthBufferPtr depthTarget, DepthStencilAspect depthStencilAspect)
{
	m_contextImpl->BeginRendering(renderTargets, depthTarget, depthStencilAspect);
}


inline void GraphicsContext::EndRendering()
{
	m_contextImpl->EndRendering();
}


inline void GraphicsContext::SetRootSignature(RootSignaturePtr rootSignature)
{
	m_contextImpl->SetRootSignature(rootSignature);
}


inline void GraphicsContext::SetGraphicsPipeline(GraphicsPipelineStatePtr graphicsPipeline)
{
	m_contextImpl->SetGraphicsPipeline(graphicsPipeline);
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
	m_contextImpl->SetConstantArray(rootIndex, numConstants, constants, 0);
}


inline void GraphicsContext::SetConstantArray(uint32_t rootIndex, uint32_t numConstants, const void* constants, uint32_t offset)
{
	m_contextImpl->SetConstantArray(rootIndex, numConstants, constants, offset);
}


inline void GraphicsContext::SetConstant(uint32_t rootIndex, uint32_t offset, DWParam val)
{
	m_contextImpl->SetConstant(rootIndex, offset, val);
}


inline void GraphicsContext::SetConstants(uint32_t rootIndex, DWParam x)
{
	m_contextImpl->SetConstants(rootIndex, x);
}


inline void GraphicsContext::SetConstants(uint32_t rootIndex, DWParam x, DWParam y)
{
	m_contextImpl->SetConstants(rootIndex, x, y);
}


inline void GraphicsContext::SetConstants(uint32_t rootIndex, DWParam x, DWParam y, DWParam z)
{
	m_contextImpl->SetConstants(rootIndex, x, y, z);
}


inline void GraphicsContext::SetConstants(uint32_t rootIndex, DWParam x, DWParam y, DWParam z, DWParam w)
{
	m_contextImpl->SetConstants(rootIndex, x, y, z, w);
}


inline void GraphicsContext::SetConstantBuffer(uint32_t rootIndex, GpuBufferPtr gpuBuffer)
{
	m_contextImpl->SetConstantBuffer(rootIndex, gpuBuffer);
}


inline void GraphicsContext::SetDescriptors(uint32_t rootIndex, DescriptorSetPtr descriptorSet)
{
	m_contextImpl->SetDescriptors(rootIndex, descriptorSet);
}


inline void GraphicsContext::SetResources(ResourceSet& resourceSet)
{
	m_contextImpl->SetResources(resourceSet);
}


inline void GraphicsContext::SetSRV(uint32_t rootParam, uint32_t offset, ColorBufferPtr colorBuffer)
{
	m_contextImpl->SetSRV(rootParam, offset, colorBuffer);
}


inline void GraphicsContext::SetSRV(uint32_t rootParam, uint32_t offset, DepthBufferPtr depthBuffer, bool depthSrv)
{
	m_contextImpl->SetSRV(rootParam, offset, depthBuffer, depthSrv);
}


inline void GraphicsContext::SetSRV(uint32_t rootParam, uint32_t offset, GpuBufferPtr gpuBuffer)
{
	m_contextImpl->SetSRV(rootParam, offset, gpuBuffer);
}


inline void GraphicsContext::SetSRV(uint32_t rootParam, uint32_t offset, TexturePtr texture)
{
	m_contextImpl->SetSRV(rootParam, offset, texture);
}


inline void GraphicsContext::SetUAV(uint32_t rootParam, uint32_t offset, ColorBufferPtr colorBuffer)
{
	m_contextImpl->SetUAV(rootParam, offset, colorBuffer);
}


inline void GraphicsContext::SetUAV(uint32_t rootParam, uint32_t offset, DepthBufferPtr depthBuffer)
{
	m_contextImpl->SetUAV(rootParam, offset, depthBuffer);
}


inline void GraphicsContext::SetUAV(uint32_t rootParam, uint32_t offset, GpuBufferPtr gpuBuffer)
{
	m_contextImpl->SetUAV(rootParam, offset, gpuBuffer);
}


inline void GraphicsContext::SetCBV(uint32_t rootParam, uint32_t offset, GpuBufferPtr gpuBuffer)
{
	m_contextImpl->SetCBV(rootParam, offset, gpuBuffer);
}


inline void GraphicsContext::SetIndexBuffer(GpuBufferPtr gpuBuffer)
{
	m_contextImpl->SetIndexBuffer(gpuBuffer);
}


inline void GraphicsContext::SetVertexBuffer(uint32_t slot, GpuBufferPtr gpuBuffer)
{
	m_contextImpl->SetVertexBuffer(slot, gpuBuffer);
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

} // namespace Luna
