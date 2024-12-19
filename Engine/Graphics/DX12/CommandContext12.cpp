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
#include "DeviceManager12.h"
#include "Queue12.h"

using namespace std;


namespace Luna::DX12
{

void ContextState::Reset()
{
	// We only call Reset() on previously freed contexts.  The command list persists, but we must
	// request a new allocator.
	assert(m_commandList != nullptr && m_currentAllocator == nullptr);
	m_currentAllocator = GetD3D12DeviceManager()->GetQueue(type).RequestAllocator();
	m_commandList->Reset(m_currentAllocator, nullptr);

	m_curGraphicsRootSignature = nullptr;
	m_curComputeRootSignature = nullptr;
	m_curPipelineState = nullptr;
	m_numBarriersToFlush = 0;

	BindDescriptorHeaps();
}


void ContextState::Initialize()
{
	GetD3D12DeviceManager()->CreateNewCommandList(type, &m_commandList, &m_currentAllocator);
}


void ContextState::BindDescriptorHeaps()
{
	// TODO
}


ComputeContext::ComputeContext()
{
	m_state.type = CommandListType::Direct;
}


GraphicsContext::GraphicsContext()
{
	m_state.type = CommandListType::Compute;
}

} // namespace Luna::DX12