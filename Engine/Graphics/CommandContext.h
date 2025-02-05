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
class GraphicsContext;

class IColorBuffer;
class IDepthBuffer;
class IGpuBuffer;
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

	virtual void TransitionResource(IGpuResource* gpuResource, ResourceState newState, bool bFlushImmediate = false) = 0;
	virtual void InsertUAVBarrier(IGpuResource* colorBuffer, bool bFlushImmediate) = 0;
	virtual void FlushResourceBarriers() = 0;

	// Graphics context
	virtual void ClearColor(IColorBuffer* colorBuffer) = 0;
	virtual void ClearColor(IColorBuffer* colorBuffer, Color clearColor) = 0;
	virtual void ClearDepth(IDepthBuffer* depthBuffer) = 0;
	virtual void ClearStencil(IDepthBuffer* depthBuffer) = 0;
	virtual void ClearDepthAndStencil(IDepthBuffer* depthBuffer) = 0;

	virtual void BeginRendering(IColorBuffer* renderTarget) = 0;
	virtual void BeginRendering(IColorBuffer* renderTarget, IDepthBuffer* depthTarget, DepthStencilAspect depthStencilAspect) = 0;
	virtual void BeginRendering(IDepthBuffer* depthTarget, DepthStencilAspect depthStencilAspect) = 0;
	virtual void BeginRendering(std::span<IColorBuffer*> renderTargets) = 0;
	virtual void BeginRendering(std::span<IColorBuffer*> renderTargets, IDepthBuffer* depthTarget, DepthStencilAspect depthStencilAspect) = 0;
	virtual void EndRendering() = 0;

	// TODO: look into the inverted viewport situation for Vulkan
	virtual void SetViewport(float x, float y, float w, float h, float minDepth, float maxDepth) = 0;
	virtual void SetScissor(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom) = 0;
	// TODO: Support separate front and back stencil
	virtual void SetStencilRef(uint32_t stencilRef) = 0;
	virtual void SetBlendFactor(Color blendFactor) = 0;
	virtual void SetPrimitiveTopology(PrimitiveTopology topology) = 0;

	// Compute context
	
protected:
	virtual void InitializeBuffer_Internal(IGpuBuffer* destBuffer, const void* bufferData, size_t numBytes, size_t offset) = 0;
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

	static void InitializeBuffer(IGpuBuffer* destBuffer, const void* bufferData, size_t numBytes, size_t offset = 0);

	// Flush existing commands and release the current context
	uint64_t Finish(bool bWaitForCompletion = false);

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

	void ClearColor(IColorBuffer* colorBuffer);
	void ClearColor(IColorBuffer* colorBuffer, Color clearColor);
	void ClearDepth(IDepthBuffer* depthBuffer);
	void ClearStencil(IDepthBuffer* depthBuffer);
	void ClearDepthAndStencil(IDepthBuffer* depthBuffer);

	void BeginRendering(IColorBuffer* renderTarget);
	void BeginRendering(IColorBuffer* renderTarget, IDepthBuffer* depthTarget, DepthStencilAspect depthStencilAspect = DepthStencilAspect::ReadWrite);
	void BeginRendering(IDepthBuffer* depthTarget, DepthStencilAspect depthStencilAspect = DepthStencilAspect::ReadWrite);
	void BeginRendering(std::span<IColorBuffer*> renderTargets);
	void BeginRendering(std::span<IColorBuffer*> renderTargets, IDepthBuffer* depthTarget, DepthStencilAspect depthStencilAspect = DepthStencilAspect::ReadWrite);
	void EndRendering();

	void SetViewport(float x, float y, float w, float h, float minDepth = 0.0f, float maxDepth = 1.0f);
	void SetScissor(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom);
	void SetViewportAndScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h);
	void SetStencilRef(uint32_t stencilRef);
	void SetBlendFactor(Color blendFactor);
	void SetPrimitiveTopology(PrimitiveTopology topology);
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


inline void CommandContext::TransitionResource(IGpuResource* gpuResource, ResourceState newState, bool bFlushImmediate)
{
	m_contextImpl->TransitionResource(gpuResource, newState, bFlushImmediate);
}


inline void CommandContext::BeginFrame()
{
	m_contextImpl->BeginFrame();
}


inline void GraphicsContext::ClearColor(IColorBuffer* colorBuffer)
{
	m_contextImpl->ClearColor(colorBuffer);
}


inline void GraphicsContext::ClearColor(IColorBuffer* colorBuffer, Color clearColor)
{
	m_contextImpl->ClearColor(colorBuffer, clearColor);
}


inline void GraphicsContext::ClearDepth(IDepthBuffer* depthBuffer)
{
	m_contextImpl->ClearDepth(depthBuffer);
}


inline void GraphicsContext::ClearStencil(IDepthBuffer* depthBuffer)
{
	m_contextImpl->ClearStencil(depthBuffer);
}


inline void GraphicsContext::ClearDepthAndStencil(IDepthBuffer* depthBuffer)
{
	m_contextImpl->ClearDepthAndStencil(depthBuffer);
}


inline void GraphicsContext::BeginRendering(IColorBuffer* renderTarget)
{
	m_contextImpl->BeginRendering(renderTarget);
}


inline void GraphicsContext::BeginRendering(IColorBuffer* renderTarget, IDepthBuffer* depthTarget, DepthStencilAspect depthStencilAspect)
{
	m_contextImpl->BeginRendering(renderTarget, depthTarget, depthStencilAspect);
}


inline void GraphicsContext::BeginRendering(IDepthBuffer* depthTarget, DepthStencilAspect depthStencilAspect)
{
	m_contextImpl->BeginRendering(depthTarget, depthStencilAspect);
}


inline void GraphicsContext::BeginRendering(std::span<IColorBuffer*> renderTargets)
{
	m_contextImpl->BeginRendering(renderTargets);
}


inline void GraphicsContext::BeginRendering(std::span<IColorBuffer*> renderTargets, IDepthBuffer* depthTarget, DepthStencilAspect depthStencilAspect)
{
	m_contextImpl->BeginRendering(renderTargets, depthTarget, depthStencilAspect);
}


inline void GraphicsContext::EndRendering()
{
	m_contextImpl->EndRendering();
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

} // namespace Luna
