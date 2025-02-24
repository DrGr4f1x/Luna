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
class ColorBufferManager;
class DepthBufferManager;
class DescriptorAllocator;
class DescriptorSetManager;
class GpuBufferManager;
class PipelineStateManager;
class Queue;
class RootSignaturePool;


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
	{
	}

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
	void CreateNewCommandList(CommandListType commandListType, ID3D12GraphicsCommandList** commandList, ID3D12CommandAllocator** allocator);
	void FreeContext(CommandContext* usedContext) final;

	ColorBuffer& GetColorBuffer() final;

	Format GetColorFormat() final;
	Format GetDepthFormat() final;

	IColorBufferManager* GetColorBufferManager() override;
	IDepthBufferManager* GetDepthBufferManager() override;
	IDescriptorSetManager* GetDescriptorSetManager() override;
	IGpuBufferManager* GetGpuBufferManager() override;
	IPipelineStateManager* GetPipelineStateManager() override;
	IRootSignaturePool* GetRootSignaturePool() override;

	// Texture formats
	uint8_t GetFormatPlaneCount(DXGI_FORMAT format);

	D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count = 1);

	void HandleDeviceLost();

	void ReleaseResource(ID3D12Resource* resource, D3D12MA::Allocation* allocation = nullptr);
	void ReleaseAllocation(D3D12MA::Allocation* allocation);

	ID3D12Device* GetDevice() { return m_dxDevice.get(); }
	D3D12MA::Allocator* GetAllocator() { return m_d3d12maAllocator.get(); }

private:
	void CreateDevice();
	void CreateResourceManagers();
	std::vector<AdapterInfo> EnumerateAdapters();
	HRESULT EnumAdapter(int32_t adapterIdx, DXGI_GPU_PREFERENCE gpuPreference, IDXGIFactory6* dxgiFactory6, IDXGIAdapter** adapter);

	Queue& GetQueue(QueueType queueType);
	Queue& GetQueue(CommandListType commandListType);

	void InstallDebugCallback();
	void ReadCaps();

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

	// Descriptor allocators
	std::array<std::unique_ptr<DescriptorAllocator>, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> m_descriptorAllocators;

	// DirectX caps
	std::unique_ptr<DeviceCaps> m_caps;

	// DirectX resource managers
	std::unique_ptr<ColorBufferManager> m_colorBufferManager;
	std::unique_ptr<DepthBufferManager> m_depthBufferManager;
	std::unique_ptr<DescriptorSetManager> m_descriptorSetManager;
	std::unique_ptr<GpuBufferManager> m_gpuBufferManager;
	std::unique_ptr<PipelineStateManager> m_pipelineStateManager;
	std::unique_ptr<RootSignaturePool> m_rootSignaturePool;

	// Swap-chain objects
	wil::com_ptr<IDXGISwapChain3> m_dxSwapChain;
	std::vector<ColorBuffer> m_swapChainBuffers;
	uint32_t m_backBufferIndex{ 0 };
	Format m_swapChainFormat;

	// Presentation synchronization
	wil::com_ptr<ID3D12Fence> m_fence;
	uint64_t m_fenceValues[3];
	Wrappers::Event m_fenceEvent;

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