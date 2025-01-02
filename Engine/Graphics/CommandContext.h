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
class ColorBuffer;
class ComputeContext;
class DepthBuffer;
class GpuResource;
class GraphicsContext;
class CommandContext;


class __declspec(uuid("ECBD0FFD-6571-4836-9DBB-7DC6436E086F")) ICommandContext : public IUnknown
{
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

	virtual void TransitionResource(ColorBuffer& colorBuffer, ResourceState newState, bool bFlushImmediate = false) = 0;
	virtual void TransitionResource(DepthBuffer& depthBuffer, ResourceState newState, bool bFlushImmediate = false) = 0;
	virtual void InsertUAVBarrier(ColorBuffer& colorBuffer, bool bFlushImmediate) = 0;
	virtual void FlushResourceBarriers() = 0;

	// Graphics context
	virtual void ClearColor(ColorBuffer& colorBuffer) = 0;
	virtual void ClearColor(ColorBuffer& colorBuffer, Color clearColor) = 0;
	virtual void ClearDepth(DepthBuffer& depthBuffer) = 0;
	virtual void ClearStencil(DepthBuffer& depthBuffer) = 0;
	virtual void ClearDepthAndStencil(DepthBuffer& depthBuffer) = 0;

	// Compute context

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

	// Flush existing commands and release the current context
	uint64_t Finish(bool bWaitForCompletion = false);

	void TransitionResource(ColorBuffer& colorBuffer, ResourceState newState, bool bFlushImmediate = false);
	void TransitionResource(DepthBuffer& depthBuffer, ResourceState newState, bool bFlushImmediate = false);

protected:
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

	void ClearColor(ColorBuffer& colorBuffer);
	void ClearColor(ColorBuffer& colorBuffer, Color clearColor);
	void ClearDepth(DepthBuffer& depthBuffer);
	void ClearStencil(DepthBuffer& depthBuffer);
	void ClearDepthAndStencil(DepthBuffer& depthBuffer);
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


inline void CommandContext::TransitionResource(ColorBuffer& colorBuffer, ResourceState newState, bool bFlushImmediate)
{
	m_contextImpl->TransitionResource(colorBuffer, newState, bFlushImmediate);
}


inline void CommandContext::TransitionResource(DepthBuffer& depthBuffer, ResourceState newState, bool bFlushImmediate)
{
	m_contextImpl->TransitionResource(depthBuffer, newState, bFlushImmediate);
}


inline void CommandContext::BeginFrame()
{
	m_contextImpl->BeginFrame();
}


inline void GraphicsContext::ClearColor(ColorBuffer& colorBuffer)
{
	m_contextImpl->ClearColor(colorBuffer);
}


inline void GraphicsContext::ClearColor(ColorBuffer& colorBuffer, Color clearColor)
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

} // namespace Luna
