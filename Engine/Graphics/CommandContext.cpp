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

#include "CommandContext.h"

#include "Application.h"
#include "GraphicsCommon.h"
#include "DeviceManager.h"

using namespace std;


namespace Luna
{

CommandContext& CommandContext::Begin(const string id)
{
	CommandContext* newContext = GetDeviceManager()->AllocateContext(CommandListType::Direct);
	newContext->SetId(id);
	newContext->BeginFrame();
	newContext->BeginEvent(id);

	// TODO
#if 0
	if (id.length() > 0)
	{
		EngineProfiling::BeginBlock(id, newContext);
	}
#endif

	return *newContext;
}


uint64_t CommandContext::Finish(bool bWaitForCompletion)
{
	m_contextImpl->EndEvent();
	uint64_t fenceValue = m_contextImpl->Finish(bWaitForCompletion);

	GetDeviceManager()->FreeContext(this);

	return fenceValue;
}


void CommandContext::InitializeBuffer(const GpuBufferPtr& destBuffer, const void* bufferData, size_t numBytes, size_t offset)
{
	CommandContext& initContext = CommandContext::Begin();

	initContext.m_contextImpl->InitializeBuffer_Internal(destBuffer.get(), bufferData, numBytes, offset);

	initContext.Finish(true);
}


void CommandContext::InitializeTexture(const TexturePtr& destTexture, const TextureInitializer& texInit)
{
	CommandContext& initContext = CommandContext::Begin();

	initContext.m_contextImpl->InitializeTexture_Internal(destTexture.Get(), texInit);

	initContext.Finish(true);
}


ComputeContext& ComputeContext::Begin(const string& id, bool bAsync)
{
	CommandListType commandListType = bAsync ? CommandListType::Compute : CommandListType::Direct;

	ComputeContext& newContext = GetDeviceManager()->AllocateContext(commandListType)->GetComputeContext();
	newContext.SetId(id);
	newContext.BeginFrame();

	// TODO
#if 0
	if (id.length() > 0)
	{
		EngineProfiling::BeginBlock(id, newContext);
	}
#endif

	//m_contextImpl->Begin

	return newContext;
}


ScopedDrawEvent::ScopedDrawEvent(CommandContext& context, const string& label)
	: m_context{ context.m_contextImpl.get() }
{
#if FRAMEPRO_ENABLED
	assert(IsFrameProRunning());
	FRAMEPRO_GET_CLOCK_COUNT(m_startTime);
	m_stringId = FramePro::RegisterString(label.c_str());
#endif

	m_context->BeginEvent(label);
}


ScopedDrawEvent::ScopedDrawEvent(ICommandContext* context, const std::string& label)
	: m_context{ context }
{
#if FRAMEPRO_ENABLED
	assert(IsFrameProRunning());
	FRAMEPRO_GET_CLOCK_COUNT(m_startTime);
	m_stringId = FramePro::RegisterString(label.c_str());
#endif

	m_context->BeginEvent(label);
}


ScopedDrawEvent::~ScopedDrawEvent()
{
	m_context->EndEvent();

#if FRAMEPRO_ENABLED
	assert(IsFrameProRunning());
	int64_t endTime;
	FRAMEPRO_GET_CLOCK_COUNT(endTime);
	int64_t duration = endTime - m_startTime;
	if (duration < 0)
	{
		return;
	}
	else if (FramePro::IsConnected() && (duration > FramePro::GetConditionalScopeMinTime()))
	{
		FramePro::AddTimeSpan(m_stringId, "none", m_startTime, endTime);
	}
#endif
}

} // namespace Luna