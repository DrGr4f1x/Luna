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

#include "Graphics\CommandContext.h"
#include "Graphics\DX12\DirectXCommon.h"

using namespace Microsoft::WRL;


namespace Luna::DX12
{

// Forward declarations
class ComputeContext;
class GraphicsContext;


struct ContextState
{
	friend class ComputeContext;
	friend class GraphicsContext;

	std::string id;
	CommandListType type;

private:
	void Reset();
	void Initialize();
	void BindDescriptorHeaps();

private:
	ID3D12GraphicsCommandList* m_commandList{ nullptr };
	ID3D12CommandAllocator* m_currentAllocator{ nullptr };

	ID3D12RootSignature* m_curGraphicsRootSignature{ nullptr };
	ID3D12RootSignature* m_curComputeRootSignature{ nullptr };
	ID3D12PipelineState* m_curPipelineState{ nullptr };

	D3D12_RESOURCE_BARRIER m_resourceBarrierBuffer[16];
	uint32_t m_numBarriersToFlush{ 0 };
};


class __declspec(uuid("54D5CC55-7AC6-4ED8-BA59-EC1264C94DE1")) ComputeContext
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IComputeContext, ICommandContext>>
	, NonCopyable
{
	friend class DeviceManager;

public:
	ComputeContext();
	virtual ~ComputeContext() = default;

	// ICommandList implementation
	void SetId(const std::string& id) final { m_state.id = id; }
	CommandListType GetType() const final { return m_state.type; }

	void Reset() final { m_state.Reset(); }
	void Initialize() final { m_state.Initialize(); }

private:
	ContextState m_state;
};


class __declspec(uuid("317216FB-BB09-4295-9157-6F4147C0C2B4")) GraphicsContext
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IGraphicsContext, IComputeContext, ICommandContext>>
	, NonCopyable
{
	friend class DeviceManager;

public:
	GraphicsContext();
	virtual ~GraphicsContext() = default;

	// ICommandList implementation
	void SetId(const std::string& id) final { m_state.id = id; }
	CommandListType GetType() const final { return m_state.type; }

	void Reset() final { m_state.Reset(); }
	void Initialize() final { m_state.Initialize(); }

private:
	ContextState m_state;
};

} // namespace Luna::DX12