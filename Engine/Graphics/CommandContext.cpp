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
	uint64_t fenceValue = m_contextImpl->Finish(bWaitForCompletion);

	GetDeviceManager()->FreeContext(this);

	return fenceValue;
}


void CommandContext::InitializeBuffer(IGpuBuffer* destBuffer, const void* bufferData, size_t numBytes, size_t offset)
{
	CommandContext& initContext = CommandContext::Begin();

	initContext.m_contextImpl->InitializeBuffer_Internal(destBuffer, bufferData, numBytes, offset);

	initContext.Finish(true);
}


ComputeContext& ComputeContext::Begin(const string id, bool bAsync)
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

} // namespace Luna