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

#include "Graphics\DeviceManager.h"
#include "Graphics\DX12\ColorBuffer12.h"
#include "Graphics\DX12\DirectXCommon.h"

using namespace Microsoft::WRL;


namespace Luna::DX12
{

// Forward declarations
struct ContextState;
struct DeviceCaps;
class GraphicsDevice;
class Queue;


struct DxgiRLOHelper
{
	bool doReport{ false };

	DxgiRLOHelper() noexcept = default;
	~DxgiRLOHelper();
};


class __declspec(uuid("2B2F2AAF-4D90-45F4-8BF8-9D8136AB6FC8")) DeviceManager 
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, Luna::IDeviceManager>
	, public NonCopyable
{
	friend struct ContextState;

public:
	DeviceManager(const DeviceManagerDesc& desc);
	virtual ~DeviceManager();

	void BeginFrame() final;
	void Present() final;

	void WaitForGpu() final;
	void WaitForFence(uint64_t fenceValue);

	void CreateDeviceResources() final;
	void CreateWindowSizeDependentResources() final;

	ICommandContext* AllocateContext(CommandListType commandListType);
	void CreateNewCommandList(CommandListType commandListType, ID3D12GraphicsCommandList** commandList, ID3D12CommandAllocator** allocator);
	void FreeContext(ICommandContext* usedContext);

	IColorBuffer* GetColorBuffer() const final;

	void HandleDeviceLost();

private:
	void CreateDevice();
	std::vector<AdapterInfo> EnumerateAdapters();
	HRESULT EnumAdapter(int32_t adapterIdx, DXGI_GPU_PREFERENCE gpuPreference, IDXGIFactory6* dxgiFactory6, IDXGIAdapter** adapter);

	Queue& GetQueue(QueueType queueType);
	Queue& GetQueue(CommandListType commandListType);

	void UpdateColorSpace();

	wil::com_ptr<ColorBuffer> CreateColorBufferFromSwapChain(uint32_t imageIndex);

private:
	DeviceManagerDesc m_desc{};
	bool m_bIsDeveloperModeEnabled{ false };
	bool m_bIsRenderDocAvailable{ false };

	DxgiRLOHelper m_dxgiRLOHelper;

	D3D_FEATURE_LEVEL m_bestFeatureLevel{ D3D_FEATURE_LEVEL_12_2 };
	D3D_SHADER_MODEL m_bestShaderModel{ D3D_SHADER_MODEL_6_7 };

	wil::com_ptr<IDXGIFactory4> m_dxgiFactory;

	// Luna objects
	wil::com_ptr<GraphicsDevice> m_device;

	// Swap-chain objects
	wil::com_ptr<IDXGISwapChain3> m_dxSwapChain;
	std::vector<wil::com_ptr<ColorBuffer>> m_swapChainBuffers;
	wil::com_ptr<ID3D12Resource> m_depthStencil;
	uint32_t m_backBufferIndex{ 0 };

	// Presentation synchronization
	wil::com_ptr<ID3D12Fence> m_fence;
	uint64_t m_fenceValues[3];
	Wrappers::Event m_fenceEvent;

	// HDR Support
	DXGI_COLOR_SPACE_TYPE m_colorSpace;

	bool m_bIsWarpAdapter{ false };
	bool m_bIsTearingSupported{ false };
	bool m_bIsAgilitySDKAvailable{ false };

	// Queues
	bool m_bQueuesCreated{ false };
	std::array<std::unique_ptr<Queue>, (uint32_t)QueueType::Count> m_queues;

	// Command context handling
	std::mutex m_contextAllocationMutex;
	std::vector<wil::com_ptr<ICommandContext>> m_contextPool[4];
	std::queue<ICommandContext*> m_availableContexts[4];
};


DeviceManager* GetD3D12DeviceManager();

} // namespace Luna::DX12