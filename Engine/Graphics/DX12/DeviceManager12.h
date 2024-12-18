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
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, Luna::DeviceManager>
	, public NonCopyable
{
public:
	DeviceManager(const DeviceManagerDesc& desc);
	virtual ~DeviceManager() = default;

	void WaitForGpu() final;

	void CreateDeviceResources() final;
	void CreateWindowSizeDependentResources() final;

	void HandleDeviceLost();

private:
	void CreateDevice();
	std::vector<AdapterInfo> EnumerateAdapters();
	HRESULT EnumAdapter(int32_t adapterIdx, DXGI_GPU_PREFERENCE gpuPreference, IDXGIFactory6* dxgiFactory6, IDXGIAdapter** adapter);

	Queue& GetQueue(QueueType queueType);
	Queue& GetQueue(CommandListType commandListType);

	void UpdateColorSpace();

private:
	DeviceManagerDesc m_desc{};
	bool m_bIsDeveloperModeEnabled{ false };
	bool m_bIsRenderDocAvailable{ false };

	DxgiRLOHelper m_dxgiRLOHelper;

	D3D_FEATURE_LEVEL m_bestFeatureLevel{ D3D_FEATURE_LEVEL_12_2 };
	D3D_SHADER_MODEL m_bestShaderModel{ D3D_SHADER_MODEL_6_7 };

	ComPtr<IDXGIFactory4> m_dxgiFactory;

	// Swap-chain objects
	ComPtr<IDXGISwapChain3> m_dxSwapChain;
	ComPtr<ID3D12Resource> m_renderTargets[3]; // TODO: wrap this in ColorBuffer/FrameBuffer
	ComPtr<ID3D12Resource> m_depthStencil;
	uint32_t m_backBufferIndex{ 0 };

	// Presentation synchronization
	ComPtr<ID3D12Fence> m_fence;
	uint64_t m_fenceValues[3];
	Wrappers::Event m_fenceEvent;

	// HDR Support
	DXGI_COLOR_SPACE_TYPE m_colorSpace;

	bool m_bIsWarpAdapter{ false };
	bool m_bIsTearingSupported{ false };
	bool m_bIsAgilitySDKAvailable{ false };

	// Luna objects
	ComPtr<GraphicsDevice> m_device;

	// Queues
	bool m_bQueuesCreated{ false };
	std::array<std::unique_ptr<Queue>, (uint32_t)QueueType::Count> m_queues;
};

} // namespace Luna::DX12