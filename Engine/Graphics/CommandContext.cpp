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

using namespace std;


namespace Luna
{

ICommandContext& CommandContext::Begin(const string id)
{
	auto deviceManager = GetDeviceManager();
	assert(deviceManager != nullptr);

	ICommandContext* newContext = deviceManager->AllocateContext(CommandListType::Direct);
	newContext->SetId(id);

	return *newContext;
}


IGraphicsContext& GraphicsContext::Begin(const string id)
{
	auto deviceManager = GetDeviceManager();
	assert(deviceManager != nullptr);

	ICommandContext* newContext = deviceManager->AllocateContext(CommandListType::Direct);
	newContext->SetId(id);

	IGraphicsContext* graphicsContext{ nullptr };
	ThrowIfFailed(newContext->QueryInterface(IID_PPV_ARGS(&graphicsContext)));

	return *graphicsContext;
}


IComputeContext& ComputeContext::Begin(const string id, bool bAsync)
{
	auto deviceManager = GetDeviceManager();
	assert(deviceManager != nullptr);

	ICommandContext* newContext = deviceManager->AllocateContext(bAsync ? CommandListType::Direct : CommandListType::Compute);
	newContext->SetId(id);

	IComputeContext* computeContext{ nullptr };
	ThrowIfFailed(newContext->QueryInterface(IID_PPV_ARGS(&computeContext)));

	return *computeContext;
}

} // namespace Luna