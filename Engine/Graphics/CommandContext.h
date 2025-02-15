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


namespace Luna
{

// Forward declarations
class CommandContext;
class ComputeContext;
class DepthBuffer;
class DescriptorSet;
class GpuBuffer;
class GraphicsContext;
class GraphicsPipelineState;
class ResourceSet;
class RootSignature;

class IColorBuffer;
class IGpuResource;


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

	virtual void TransitionResource(DepthBuffer& depthBuffer, ResourceState newState, bool bFlushImmediate = false) = 0;
	virtual void TransitionResource(GpuBuffer& gpuBuffer, ResourceState newState, bool bFlushImmediate = false) = 0;
	virtual void TransitionResource(IGpuResource* gpuResource, ResourceState newState, bool bFlushImmediate = false) = 0;
	virtual void InsertUAVBarrier(IGpuResource* colorBuffer, bool bFlushImmediate) = 0;
	virtual void FlushResourceBarriers() = 0;

	// Graphics context
	virtual void ClearUAV(GpuBuffer& gpuBuffer) = 0;
	// TODO: Figure out how to implement this for Vulkan
	//virtual void ClearUAV(IColorBuffer* colorBuffer) = 0;
	virtual void ClearColor(IColorBuffer* colorBuffer) = 0;
	virtual void ClearColor(IColorBuffer* colorBuffer, Color clearColor) = 0;
	virtual void ClearDepth(DepthBuffer& depthBuffer) = 0;
	virtual void ClearStencil(DepthBuffer& depthBuffer) = 0;
	virtual void ClearDepthAndStencil(DepthBuffer& depthBuffer) = 0;

	virtual void BeginRendering(IColorBuffer* renderTarget) = 0;
	virtual void BeginRendering(IColorBuffer* renderTarget, DepthBuffer& depthTarget, DepthStencilAspect depthStencilAspect) = 0;
	virtual void BeginRendering(DepthBuffer& depthTarget, DepthStencilAspect depthStencilAspect) = 0;
	virtual void BeginRendering(std::span<IColorBuffer*> renderTargets) = 0;
	virtual void BeginRendering(std::span<IColorBuffer*> renderTargets, DepthBuffer& depthTarget, DepthStencilAspect depthStencilAspect) = 0;
	virtual void EndRendering() = 0;

	virtual void SetRootSignature(RootSignature& rootSignature) = 0;
	virtual void SetGraphicsPipeline(GraphicsPipelineState& graphicsPipeline) = 0;

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
	virtual void SetDescriptors(uint32_t rootIndex, DescriptorSet& descriptorSet) = 0;
	virtual void SetResources(ResourceSet& resourceSet) = 0;

	virtual void SetIndexBuffer(const GpuBuffer& gpuBuffer) = 0;
	virtual void SetVertexBuffer(uint32_t slot, const GpuBuffer& gpuBuffer) = 0;

	virtual void DrawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount,
		uint32_t startVertexLocation, uint32_t startInstanceLocation) = 0;
	virtual void DrawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation,
		int32_t baseVertexLocation, uint32_t startInstanceLocation) = 0;

	// Compute context
	
protected:
	virtual void InitializeBuffer_Internal(GpuBuffer& destBuffer, const void* bufferData, size_t numBytes, size_t offset) = 0;
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

	static void InitializeBuffer(GpuBuffer& destBuffer, const void* bufferData, size_t numBytes, size_t offset = 0);

	// Flush existing commands and release the current context
	uint64_t Finish(bool bWaitForCompletion = false);

	void TransitionResource(DepthBuffer& depthBuffer, ResourceState newState, bool bFlushImmediate = false);
	void TransitionResource(GpuBuffer& gpuBuffer, ResourceState newState, bool bFlushImmediate = false);
	void TransitionResource(IGpuResource* gpuResource, ResourceState newState, bool bFlushImmediate = false);
	
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

	void ClearUAV(GpuBuffer& gpuBuffer);
	//void ClearUAV(IColorBuffer* colorBuffer);
	void ClearColor(IColorBuffer* colorBuffer);
	void ClearColor(IColorBuffer* colorBuffer, Color clearColor);
	void ClearDepth(DepthBuffer& depthBuffer);
	void ClearStencil(DepthBuffer& depthBuffer);
	void ClearDepthAndStencil(DepthBuffer& depthBuffer);

	void BeginRendering(IColorBuffer* renderTarget);
	void BeginRendering(IColorBuffer* renderTarget, DepthBuffer& depthTarget, DepthStencilAspect depthStencilAspect = DepthStencilAspect::ReadWrite);
	void BeginRendering(DepthBuffer& depthTarget, DepthStencilAspect depthStencilAspect = DepthStencilAspect::ReadWrite);
	void BeginRendering(std::span<IColorBuffer*> renderTargets);
	void BeginRendering(std::span<IColorBuffer*> renderTargets, DepthBuffer& depthTarget, DepthStencilAspect depthStencilAspect = DepthStencilAspect::ReadWrite);
	void EndRendering();

	void SetRootSignature(RootSignature& rootSignature);
	void SetGraphicsPipeline(GraphicsPipelineState& graphicsPipeline);

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
	void SetDescriptors(uint32_t rootIndex, DescriptorSet& descriptorSet);
	void SetResources(ResourceSet& resourceSet);

	void SetIndexBuffer(const GpuBuffer& gpuBuffer);
	void SetVertexBuffer(uint32_t slot, const GpuBuffer& gpuBuffer);

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


inline void CommandContext::TransitionResource(DepthBuffer& depthBuffer, ResourceState newState, bool bFlushImmediate)
{
	m_contextImpl->TransitionResource(depthBuffer, newState, bFlushImmediate);
}


inline void CommandContext::TransitionResource(GpuBuffer& gpuBuffer, ResourceState newState, bool bFlushImmediate)
{
	m_contextImpl->TransitionResource(gpuBuffer, newState, bFlushImmediate);
}


inline void CommandContext::TransitionResource(IGpuResource* gpuResource, ResourceState newState, bool bFlushImmediate)
{
	m_contextImpl->TransitionResource(gpuResource, newState, bFlushImmediate);
}


inline void CommandContext::BeginFrame()
{
	m_contextImpl->BeginFrame();
}


inline void GraphicsContext::ClearUAV(GpuBuffer& gpuBuffer)
{
	m_contextImpl->ClearUAV(gpuBuffer);
}


//inline void GraphicsContext::ClearUAV(IColorBuffer* colorBuffer)
//{
//	m_contextImpl->ClearUAV(colorBuffer);
//}


inline void GraphicsContext::ClearColor(IColorBuffer* colorBuffer)
{
	m_contextImpl->ClearColor(colorBuffer);
}


inline void GraphicsContext::ClearColor(IColorBuffer* colorBuffer, Color clearColor)
{
	m_contextImpl->ClearColor(colorBuffer, clearColor);
}


inline void GraphicsContext::ClearDepth(DepthBuffer& depthBuffer)
{
	m_contextImpl->ClearDepth(depthBuffer);
}


inline void GraphicsContext::ClearStencil(DepthBuffer& depthBuffer)
{
	m_contextImpl->ClearStencil(depthBuffer);
}


inline void GraphicsContext::ClearDepthAndStencil(DepthBuffer& depthBuffer)
{
	m_contextImpl->ClearDepthAndStencil(depthBuffer);
}


inline void GraphicsContext::BeginRendering(IColorBuffer* renderTarget)
{
	m_contextImpl->BeginRendering(renderTarget);
}


inline void GraphicsContext::BeginRendering(IColorBuffer* renderTarget, DepthBuffer& depthTarget, DepthStencilAspect depthStencilAspect)
{
	m_contextImpl->BeginRendering(renderTarget, depthTarget, depthStencilAspect);
}


inline void GraphicsContext::BeginRendering(DepthBuffer& depthTarget, DepthStencilAspect depthStencilAspect)
{
	m_contextImpl->BeginRendering(depthTarget, depthStencilAspect);
}


inline void GraphicsContext::BeginRendering(std::span<IColorBuffer*> renderTargets)
{
	m_contextImpl->BeginRendering(renderTargets);
}


inline void GraphicsContext::BeginRendering(std::span<IColorBuffer*> renderTargets, DepthBuffer& depthTarget, DepthStencilAspect depthStencilAspect)
{
	m_contextImpl->BeginRendering(renderTargets, depthTarget, depthStencilAspect);
}


inline void GraphicsContext::EndRendering()
{
	m_contextImpl->EndRendering();
}


inline void GraphicsContext::SetRootSignature(RootSignature& rootSignature)
{
	m_contextImpl->SetRootSignature(rootSignature);
}


inline void GraphicsContext::SetGraphicsPipeline(GraphicsPipelineState& graphicsPipeline)
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


inline void GraphicsContext::SetDescriptors(uint32_t rootIndex, DescriptorSet& descriptorSet)
{
	m_contextImpl->SetDescriptors(rootIndex, descriptorSet);
}


inline void GraphicsContext::SetResources(ResourceSet& resourceSet)
{
	m_contextImpl->SetResources(resourceSet);
}


inline void GraphicsContext::SetIndexBuffer(const GpuBuffer& gpuBuffer)
{
	m_contextImpl->SetIndexBuffer(gpuBuffer);
}


inline void GraphicsContext::SetVertexBuffer(uint32_t slot, const GpuBuffer& gpuBuffer)
{
	m_contextImpl->SetVertexBuffer(slot, gpuBuffer);
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
