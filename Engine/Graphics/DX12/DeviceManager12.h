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

#include "Graphics\ColorBuffer.h"
#include "Graphics\DeviceManager.h"
#include "Graphics\DX12\DirectXCommon.h"

using namespace Microsoft::WRL;


namespace Luna::DX12
{

// Forward declarations
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
	friend class CommandContext12;

public:
	DeviceManager(const DeviceManagerDesc& desc);
	virtual ~DeviceManager();

	void BeginFrame() final;
	void Present() final;

	void WaitForGpu() final;
	void WaitForFence(uint64_t fenceValue);

	void SetWindowSize(uint32_t width, uint32_t height) final;
	void CreateDeviceResources() final;
	void CreateWindowSizeDependentResources() final;

	CommandContext* AllocateContext(CommandListType commandListType) final;
	void CreateNewCommandList(CommandListType commandListType, ID3D12GraphicsCommandList** commandList, ID3D12CommandAllocator** allocator);
	void FreeContext(CommandContext* usedContext) final;

	ColorBufferHandle CreateColorBufferFromSwapChain(uint32_t imageIndex) final;

	ColorBufferHandle GetColorBuffer() final;

	Format GetColorFormat() final;
	Format GetDepthFormat() final;

	void HandleDeviceLost();

	void ReleaseResource(ID3D12Resource* resource, D3D12MA::Allocation* allocation = nullptr);
	void ReleaseAllocation(D3D12MA::Allocation* allocation);

private:
	void CreateDevice();
	std::vector<AdapterInfo> EnumerateAdapters();
	HRESULT EnumAdapter(int32_t adapterIdx, DXGI_GPU_PREFERENCE gpuPreference, IDXGIFactory6* dxgiFactory6, IDXGIAdapter** adapter);

	Queue& GetQueue(QueueType queueType);
	Queue& GetQueue(CommandListType commandListType);

	void UpdateColorSpace();

	void ResizeSwapChain();
	void ReleaseSwapChainBuffers();
	void ReleaseDeferredResources();

private:
	DeviceManagerDesc m_desc{};
	bool m_bIsDeveloperModeEnabled{ false };
	bool m_bIsRenderDocAvailable{ false };

	DxgiRLOHelper m_dxgiRLOHelper;

	D3D_FEATURE_LEVEL m_bestFeatureLevel{ D3D_FEATURE_LEVEL_12_2 };
	D3D_SHADER_MODEL m_bestShaderModel{ D3D_SHADER_MODEL_6_7 };

	wil::com_ptr<IDXGIFactory4> m_dxgiFactory;
	wil::com_ptr<IDXGIAdapter> m_dxgiAdapter;

	// Luna objects
	wil::com_ptr<GraphicsDevice> m_device;

	// Swap-chain objects
	wil::com_ptr<IDXGISwapChain3> m_dxSwapChain;
	std::vector<ColorBufferHandle> m_swapChainBuffers;
	wil::com_ptr<ID3D12Resource> m_depthStencil;
	uint32_t m_backBufferIndex{ 0 };
	Format m_swapChainFormat;

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
	std::vector<std::unique_ptr<CommandContext>> m_contextPool[4];
	std::queue<CommandContext*> m_availableContexts[4];

	// Deferred resource release
	struct DeferredReleaseResource
	{
		uint64_t fenceValue;
		wil::com_ptr<ID3D12Resource> resource;
		wil::com_ptr<D3D12MA::Allocation> allocation;
	};
	std::list<DeferredReleaseResource> m_deferredResources;
};


DeviceManager* GetD3D12DeviceManager();

} // namespace Luna::DX12