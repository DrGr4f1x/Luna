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
#include "Graphics\Texture.h"
#include "Graphics\DX12\DirectXCommon.h"

using namespace Microsoft::WRL;


namespace Luna::DX12
{

// Forward declarations
class DescriptorAllocator;
class Device;
class Queue;


struct DxgiRLOHelper
{
	bool doReport{ false };

	DxgiRLOHelper() noexcept = default;
	~DxgiRLOHelper();
};


struct DeviceRLDOHelper
{
	ID3D12Device* device{ nullptr };
	const bool doReport{ false };

	DeviceRLDOHelper(bool bDoReport) noexcept
		: doReport{ bDoReport }
	{}

	~DeviceRLDOHelper();
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
	bool IsFenceComplete(uint64_t fenceValue);

	void SetWindowSize(uint32_t width, uint32_t height) final;
	void CreateDeviceResources() final;
	void CreateWindowSizeDependentResources() final;

	CommandContext* AllocateContext(CommandListType commandListType) final;
	void CreateNewCommandList(
		CommandListType commandListType, 
		ID3D12GraphicsCommandList** commandList, 
		ID3D12CommandAllocator** allocator,
		ID3D12CommandSignature** drawIndirectSignature,
		ID3D12CommandSignature** drawIndexedIndirectSignature,
		ID3D12CommandSignature** dispatchIndirectSignature);
	void FreeContext(CommandContext* usedContext) final;

	GraphicsApi GetGraphicsApi() const override { return GraphicsApi::D3D12; }

	Luna::ColorBufferPtr GetColorBuffer() const final;

	Format GetColorFormat() const final;
	Format GetDepthFormat() const final;

	const std::string& GetDeviceName() const override;

	uint32_t GetNumSwapChainBuffers() const override { return m_desc.numSwapChainBuffers; }
	uint32_t GetActiveFrame() const override { return m_backBufferIndex; }
	uint64_t GetFrameNumber() const override { return m_frameNumber; }

	IDevice* GetDevice() override;

	// Queue timestamp frequency
	uint64_t GetTimestampFrequency() const { return m_timestampFrequency; }

	// Texture formats
	uint8_t GetFormatPlaneCount(DXGI_FORMAT format);

	void HandleDeviceLost();

	void ReleaseResource(ID3D12Resource* resource, D3D12MA::Allocation* allocation = nullptr);
	void ReleaseAllocation(D3D12MA::Allocation* allocation);

	ID3D12Device* GetD3D12Device() { return m_dxDevice.get(); }
	D3D12MA::Allocator* GetAllocator() { return m_d3d12maAllocator.get(); }

private:
	void CreateDevice();
	std::vector<AdapterInfo> EnumerateAdapters();
	HRESULT EnumAdapter(int32_t adapterIdx, DXGI_GPU_PREFERENCE gpuPreference, IDXGIFactory6* dxgiFactory6, IDXGIAdapter** adapter);

	Queue& GetQueue(QueueType queueType);
	Queue& GetQueue(CommandListType commandListType);

	void CreateCommandSignatures();

	void InstallDebugCallback();

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

	// DirectX objects
	wil::com_ptr<IDXGIFactory4> m_dxgiFactory;
	wil::com_ptr<IDXGIAdapter> m_dxgiAdapter;
	wil::com_ptr<ID3D12Device> m_dxDevice;
	DeviceRLDOHelper m_deviceRLDOHelper;
	wil::com_ptr<ID3D12InfoQueue1> m_dxInfoQueue;
	wil::com_ptr<D3D12MA::Allocator> m_d3d12maAllocator;
	DWORD m_callbackCookie{ 0 };

	std::string m_deviceName;

	// DirectX device wrapper
	std::unique_ptr<Device> m_device;

	// Texture manager
	std::unique_ptr<TextureManager> m_textureManager;

	// Swap-chain objects
	wil::com_ptr<IDXGISwapChain3> m_dxSwapChain;
	std::vector<ColorBufferPtr> m_swapChainBuffers;
	uint32_t m_backBufferIndex{ 0 };
	Format m_swapChainFormat;

	// Presentation synchronization
	wil::com_ptr<ID3D12Fence> m_fence;
	uint64_t m_fenceValues[3];
	Wrappers::Event m_fenceEvent;
	uint64_t m_frameNumber{ 0 };

	// HDR Support
	DXGI_COLOR_SPACE_TYPE m_colorSpace;

	// Format properties
	std::unordered_map<DXGI_FORMAT, uint8_t> m_dxgiFormatPlaneCounts;

	bool m_bIsWarpAdapter{ false };
	bool m_bIsTearingSupported{ false };
	bool m_bIsAgilitySDKAvailable{ false };

	// Queues
	bool m_bQueuesCreated{ false };
	std::array<std::unique_ptr<Queue>, (uint32_t)QueueType::Count> m_queues;
	uint64_t m_timestampFrequency{ 0 };

	// Command context handling
	std::mutex m_contextAllocationMutex;
	std::vector<std::unique_ptr<CommandContext>> m_contextPool[4];
	std::queue<CommandContext*> m_availableContexts[4];

	// Deferred resource release
	std::mutex m_deferredReleaseMutex;
	struct DeferredReleaseResource
	{
		uint64_t fenceValue;
		wil::com_ptr<ID3D12Resource> resource;
		wil::com_ptr<D3D12MA::Allocation> allocation;
	};
	std::list<DeferredReleaseResource> m_deferredResources;

	// Indirect command signatures
	wil::com_ptr<ID3D12CommandSignature> m_drawIndirectSignature;
	wil::com_ptr<ID3D12CommandSignature> m_drawIndexedIndirectSignature;
	wil::com_ptr<ID3D12CommandSignature> m_dispatchIndirectSignature;
};


DeviceManager* GetD3D12DeviceManager();

} // namespace Luna::DX12