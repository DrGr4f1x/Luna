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

#include "DeviceManager12.h"

#include "Graphics\GraphicsCommon.h"
#include "Graphics\DX12\ColorBuffer12.h"
#include "Graphics\DX12\CommandContext12.h"
#include "Graphics\DX12\DeviceCaps12.h"
#include "Graphics\DX12\Device12.h"
#include "Graphics\DX12\Queue12.h"

using namespace std;
using namespace Microsoft::WRL;


namespace Luna::DX12
{

DeviceManager* g_d3d12DeviceManager{ nullptr };

bool IsDirectXAgilitySDKAvailable()
{
	HMODULE agilitySDKDllHandle = ::GetModuleHandle("D3D12Core.dll");
	return agilitySDKDllHandle != nullptr;
}


bool TestCreateDevice(IDXGIAdapter* adapter, D3D_FEATURE_LEVEL minFeatureLevel, DeviceBasicCaps& deviceBasicCaps)
{
	wil::com_ptr<ID3D12Device> device;
	if (SUCCEEDED(D3D12CreateDevice(adapter, minFeatureLevel, IID_PPV_ARGS(&device))))
	{
		DeviceCaps testCaps;
		testCaps.ReadBasicCaps(device.get(), minFeatureLevel);
		deviceBasicCaps = testCaps.basicCaps;

		return true;
	}
	return false;
}


AdapterType GetAdapterType(IDXGIAdapter* adapter)
{
	wil::com_ptr<IDXGIAdapter3> adapter3;
	adapter->QueryInterface(IID_PPV_ARGS(&adapter3));

	// Check for integrated adapter
	DXGI_QUERY_VIDEO_MEMORY_INFO nonLocalVideoMemoryInfo{};
	if (adapter3 && SUCCEEDED(adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &nonLocalVideoMemoryInfo)))
	{
		if (nonLocalVideoMemoryInfo.Budget == 0)
		{
			return AdapterType::Integrated;
		}
	}

	// Check for software adapter (WARP)
	DXGI_ADAPTER_DESC desc{};
	adapter->GetDesc(&desc);

	if (desc.VendorId == 0x1414)
	{
		return AdapterType::Software;
	}

	return AdapterType::Discrete;
}


inline long ComputeIntersectionArea(
	long ax1, long ay1, long ax2, long ay2,
	long bx1, long by1, long bx2, long by2) noexcept
{
	return std::max(0l, std::min(ax2, bx2) - std::max(ax1, bx1)) * std::max(0l, std::min(ay2, by2) - std::max(ay1, by1));
}


DxgiRLOHelper::~DxgiRLOHelper()
{
	if (doReport)
	{
		wil::com_ptr<IDXGIDebug1> pDebug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
		{
			pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
		}
	}
}


DeviceManager::DeviceManager(const DeviceManagerDesc& desc)
	: m_desc{ desc }
{
	m_bIsDeveloperModeEnabled = IsDeveloperModeEnabled();
	m_bIsRenderDocAvailable = IsRenderDocAvailable();

	m_fenceValues[0] = 0;
	m_fenceValues[1] = 0;
	m_fenceValues[2] = 0;

	extern Luna::IDeviceManager* g_deviceManager;
	assert(!g_deviceManager);

	g_deviceManager = this;
	g_d3d12DeviceManager = this;
}


DeviceManager::~DeviceManager()
{
	WaitForGpu();

	// Flush pending deferred resources here
	ReleaseDeferredResources();
	assert(m_deferredResources.empty());

	extern Luna::IDeviceManager* g_deviceManager;
	g_deviceManager = nullptr;
	g_d3d12DeviceManager = nullptr;
}


void DeviceManager::BeginFrame()
{
	// TODO Handle window resize here

	// Schedule a Signal command in the queue.
	/*const UINT64 currentFenceValue = m_fenceValues[m_backBufferIndex];
	ThrowIfFailed(GetQueue(QueueType::Graphics).GetCommandQueue()->Signal(m_fence.get(), currentFenceValue));*/

	m_backBufferIndex = m_dxSwapChain->GetCurrentBackBufferIndex();
	
	GetQueue(CommandListType::Direct).WaitForFence(m_fenceValues[m_backBufferIndex]);

	//// If the next frame is not ready to be rendered yet, wait until it is ready.
	//if (m_fence->GetCompletedValue() < m_fenceValues[m_backBufferIndex])
	//{
	//	ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_backBufferIndex], m_fenceEvent.Get()));
	//	std::ignore = WaitForSingleObjectEx(m_fenceEvent.Get(), INFINITE, FALSE);
	//}

	//// Set the fence value for the next frame.
	//m_fenceValues[m_backBufferIndex] = currentFenceValue + 1;
}


void DeviceManager::Present()
{
	UINT vsync = m_bIsTearingSupported ? 0 : (m_desc.enableVSync ? 1 : 0);
	UINT presentFlags = m_bIsTearingSupported ? DXGI_PRESENT_ALLOW_TEARING : 0;

	m_dxSwapChain->Present(vsync, presentFlags);

	m_fenceValues[m_backBufferIndex] = GetQueue(CommandListType::Direct).GetLastSubmittedFenceValue();

	ReleaseDeferredResources();

	// TODO Handle device removed
}


void DeviceManager::WaitForGpu()
{
	if (m_bQueuesCreated)
	{
		m_queues[(uint32_t)QueueType::Graphics]->WaitForGpu();
		m_queues[(uint32_t)QueueType::Compute]->WaitForGpu();
		m_queues[(uint32_t)QueueType::Copy]->WaitForGpu();
	}
}


void DeviceManager::WaitForFence(uint64_t fenceValue)
{
	Queue& producer = GetQueue((CommandListType)(fenceValue >> 56));
	producer.WaitForFence(fenceValue);
}


void DeviceManager::SetWindowSize(uint32_t width, uint32_t height)
{
	if (m_desc.backBufferWidth != width || m_desc.backBufferHeight != height)
	{
		m_desc.backBufferWidth = width;
		m_desc.backBufferHeight = height;

		ResizeSwapChain();
	}
}


void DeviceManager::CreateDeviceResources()
{
	m_dxgiRLOHelper.doReport = m_desc.enableValidation;

	if (m_desc.enableValidation)
	{
		wil::com_ptr<ID3D12Debug> debugInterface;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface))))
		{
			debugInterface->EnableDebugLayer();
			LogInfo(LogDirectX) << "Enabled D3D12 debug layer." << endl;
		}
		else
		{
			LogWarning(LogDirectX) << "Failed to enable D3D12 debug layer." << endl;
		}
	}

	if (m_bIsDeveloperModeEnabled && !m_bIsRenderDocAvailable)
	{
		if (SUCCEEDED(D3D12EnableExperimentalFeatures(1, &D3D12ExperimentalShaderModels, nullptr, nullptr)))
		{
			LogInfo(LogDirectX) << "Enabled D3D12 experimental shader models" << endl;
		}
		else
		{
			LogWarning(LogDirectX) << "Failed to enable D3D12 experimental shader models" << endl;
		}
	}

	if (!m_dxgiFactory)
	{
		if (!SUCCEEDED(CreateDXGIFactory2(m_desc.enableValidation ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(&m_dxgiFactory))))
		{
			LogFatal(LogDirectX) << "Failed to create DXGIFactory2." << endl;
			return;
		}
	}
	SetDebugName(m_dxgiFactory.get(), "DXGI Factory");

	wil::com_ptr<IDXGIFactory5> dxgiFactory5;
	if (SUCCEEDED(m_dxgiFactory->QueryInterface(IID_PPV_ARGS(&dxgiFactory5))))
	{
		BOOL supported{ 0 };
		if (SUCCEEDED(dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &supported, sizeof(supported))))
		{
			m_bIsTearingSupported = (supported != 0);
		}
	}

	CreateDevice();
	
	// Create queues
	m_queues[(uint32_t)QueueType::Graphics] = make_unique<Queue>(m_device->GetD3D12Device(), QueueType::Graphics);
	m_queues[(uint32_t)QueueType::Compute] = make_unique<Queue>(m_device->GetD3D12Device(), QueueType::Compute);
	m_queues[(uint32_t)QueueType::Copy] = make_unique<Queue>(m_device->GetD3D12Device(), QueueType::Copy);
	m_bQueuesCreated = true;

	// Create a fence for tracking GPU execution progress
	auto d3d12Device = m_device->GetD3D12Device();
	ThrowIfFailed(d3d12Device->CreateFence(m_fenceValues[m_backBufferIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));

	m_fenceValues[m_backBufferIndex]++;

	m_fence->SetName(L"DeviceResources");

	m_fenceEvent.Attach(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));
}


void DeviceManager::CreateWindowSizeDependentResources()
{ 
	assert(m_desc.hwnd);

	WaitForGpu();

	// Release resources tied to swap chain and update fence values
	// TODO: hard-coded backbuffer count is 3 here.  Get this from somewhere else.
	const uint32_t backBufferCount{ 3 };
	m_swapChainBuffers.clear();

	// TODO: The window dimensions might have changed externally.  Need to pass those in from somewhere else.
	const uint32_t newBackBufferWidth = m_desc.backBufferWidth;
	const uint32_t newBackBufferHeight = m_desc.backBufferHeight;

	m_swapChainFormat = RemoveSrgb(m_desc.swapChainFormat);
	DXGI_FORMAT dxgiFormat = FormatToDxgi(m_swapChainFormat).rtvFormat;

	if (m_dxSwapChain)
	{
		auto hr = m_dxSwapChain->ResizeBuffers(
			backBufferCount,
			newBackBufferWidth,
			newBackBufferHeight,
			dxgiFormat,
			(m_bIsTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u));

		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			LogWarning(LogDirectX) << format("Device lost on ResizeBuffers: Reason code {}",
				static_cast<uint32_t>((hr == DXGI_ERROR_DEVICE_REMOVED) ? m_device->GetD3D12Device()->GetDeviceRemovedReason() : hr)) << endl;

			// If the device was removed for any reason, a new device and swap chain will need to be created.
			HandleDeviceLost();

			// Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method
			// and correctly set up the new device.
			return;
		}
		else
		{
			ThrowIfFailed(hr);
		}
	}
	else
	{
		auto swapChainDesc = DXGI_SWAP_CHAIN_DESC1{
			.Width			= newBackBufferWidth,
			.Height			= newBackBufferHeight,
			.Format			= dxgiFormat,
			.SampleDesc		= { .Count = 1, .Quality = 0 },
			.BufferUsage	= DXGI_USAGE_RENDER_TARGET_OUTPUT,
			.BufferCount	= backBufferCount,
			.Scaling		= DXGI_SCALING_STRETCH,
			.SwapEffect		= DXGI_SWAP_EFFECT_FLIP_DISCARD,
			.AlphaMode		= DXGI_ALPHA_MODE_IGNORE,
			.Flags			= m_bIsTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u
		};

		auto fsSwapChainDesc = DXGI_SWAP_CHAIN_FULLSCREEN_DESC{ 
			.Windowed		= TRUE
		};

		// Create a swap chain for the window.
		wil::com_ptr<IDXGISwapChain1> swapChain;
		ThrowIfFailed(m_dxgiFactory->CreateSwapChainForHwnd(
			GetQueue(QueueType::Graphics).GetCommandQueue(),
			m_desc.hwnd,
			&swapChainDesc,
			&fsSwapChainDesc,
			nullptr,
			swapChain.addressof()
		));

		m_dxSwapChain = swapChain.query<IDXGISwapChain3>();
		assert(m_dxSwapChain);

		// This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
		ThrowIfFailed(m_dxgiFactory->MakeWindowAssociation(m_desc.hwnd, DXGI_MWA_NO_ALT_ENTER));
	}

	// Handle color space settings for HDR
	UpdateColorSpace();

	// Obtain the back buffers for this window which will be the final render targets
    // and create render target views for each of them.
	for (uint32_t i = 0; i < backBufferCount; ++i)
	{
		ColorBufferHandle swapChainBuffer = CreateColorBufferFromSwapChain(i);
		m_swapChainBuffers.emplace_back(swapChainBuffer);
	}

	// Reset the index to the current back buffer.
	m_backBufferIndex = m_dxSwapChain->GetCurrentBackBufferIndex();
}


CommandContext* DeviceManager::AllocateContext(CommandListType commandListType)
{
	lock_guard<mutex> lockGuard(m_contextAllocationMutex);

	D3D12_COMMAND_LIST_TYPE d3d12Type = CommandListTypeToDX12(commandListType);

	auto& availableContexts = m_availableContexts[d3d12Type];

	CommandContext* ret{ nullptr };
	if (availableContexts.empty())
	{
		wil::com_ptr<ICommandContext> contextImpl = Make<CommandContext12>(commandListType);
		ret = new CommandContext(contextImpl.get());

		m_contextPool[d3d12Type].emplace_back(ret);
		ret->Initialize();
	}
	else
	{
		ret = availableContexts.front();
		availableContexts.pop();
		ret->Reset();
	}

	assert(ret != nullptr);
	assert(ret->GetType() == commandListType);

	return ret;
}


void DeviceManager::CreateNewCommandList(CommandListType commandListType, ID3D12GraphicsCommandList** commandList, ID3D12CommandAllocator** allocator)
{
	assert_msg(commandListType != CommandListType::Bundle, "Bundles are not yet supported");

	*allocator = GetQueue(commandListType).RequestAllocator();

	assert_succeeded(m_device->GetD3D12Device()->CreateCommandList(1, CommandListTypeToDX12(commandListType), *allocator, nullptr, IID_PPV_ARGS(commandList)));

	(*commandList)->SetName(L"CommandList");
}


void DeviceManager::FreeContext(CommandContext* usedContext)
{
	assert(usedContext != nullptr);
	lock_guard<mutex> lockGuard(m_contextAllocationMutex);

	m_availableContexts[(uint32_t)usedContext->GetType()].push(usedContext);
}


ColorBufferHandle DeviceManager::CreateColorBufferFromSwapChain(uint32_t imageIndex)
{
	wil::com_ptr<ID3D12Resource> displayPlane;
	assert_succeeded(m_dxSwapChain->GetBuffer(imageIndex, IID_PPV_ARGS(&displayPlane)));

	const string name = format("Primary SwapChain Image {}", imageIndex);
	SetDebugName(displayPlane.get(), name);

	D3D12_RESOURCE_DESC resourceDesc = displayPlane->GetDesc();

	ColorBufferDesc colorBufferDesc{
		.name				= name,
		.resourceType		= ResourceType::Texture2D,
		.width				= resourceDesc.Width,
		.height				= resourceDesc.Height,
		.arraySizeOrDepth	= resourceDesc.DepthOrArraySize,
		.numSamples			= resourceDesc.SampleDesc.Count,
		.format				= DxgiToFormat(resourceDesc.Format)
	};

	auto d3d12Device = m_device->GetD3D12Device();

	auto rtvHandle = m_device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	d3d12Device->CreateRenderTargetView(displayPlane.get(), nullptr, rtvHandle);

	auto srvHandle = m_device->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	d3d12Device->CreateShaderResourceView(displayPlane.get(), nullptr, srvHandle);

	const uint8_t planeCount = m_device->GetFormatPlaneCount(resourceDesc.Format);
	colorBufferDesc.planeCount = planeCount;

	auto colorBufferDescExt = ColorBufferDescExt{}
		.SetResource(displayPlane.get())
		.SetUsageState(ResourceState::Present)
		.SetPlaneCount(planeCount)
		.SetRtvHandle(rtvHandle)
		.SetSrvHandle(srvHandle);

	return Make<ColorBuffer12>(colorBufferDesc, colorBufferDescExt);
}


ColorBufferHandle DeviceManager::GetColorBuffer()
{
	return m_swapChainBuffers[m_backBufferIndex];
}


Format DeviceManager::GetColorFormat()
{
	return m_swapChainFormat;
}


Format DeviceManager::GetDepthFormat()
{
	return m_desc.depthBufferFormat;
}


void DeviceManager::HandleDeviceLost()
{ 
	// TODO
	/*if (m_deviceNotify)
	{
		m_deviceNotify->OnDeviceLost();
	}*/

	m_swapChainBuffers.clear();
	m_depthStencil.reset();

	m_queues[(uint32_t)QueueType::Graphics].reset();
	m_queues[(uint32_t)QueueType::Compute].reset();
	m_queues[(uint32_t)QueueType::Copy].reset();

	m_device.reset();

	m_fence.reset();
	m_dxSwapChain.reset();
	m_dxgiFactory.reset();

#if _DEBUG
	{
		wil::com_ptr<IDXGIDebug1> dxgiDebug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
		{
			dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
		}
	}
#endif

	CreateDeviceResources();
	CreateWindowSizeDependentResources();

	// TODO
	/*if(m_deviceNotify)
	{
		m_deviceNotify->OnDeviceRestored();
	}*/
}

void DeviceManager::ReleaseResource(ID3D12Resource* resource, D3D12MA::Allocation* allocation)
{
	uint64_t nextFence = GetQueue(QueueType::Graphics).GetNextFenceValue();

	DeferredReleaseResource deferredResource{ nextFence, resource, allocation };
	m_deferredResources.emplace_back(deferredResource);
}


void DeviceManager::ReleaseAllocation(D3D12MA::Allocation* allocation)
{
	uint64_t nextFence = GetQueue(QueueType::Graphics).GetNextFenceValue();

	DeferredReleaseResource deferredResource{ nextFence, nullptr, allocation };
	m_deferredResources.emplace_back(deferredResource);
}


void DeviceManager::CreateDevice()
{
	vector<AdapterInfo> adapterInfos = EnumerateAdapters();

	int32_t firstDiscreteAdapterIdx{ -1 };
	int32_t bestMemoryAdapterIdx{ -1 };
	int32_t firstAdapterIdx{ -1 };
	int32_t warpAdapterIdx{ -1 };
	int32_t chosenAdapterIdx{ -1 };
	size_t maxMemory{ 0 };

	int32_t adapterIdx{ 0 };
	for (const auto& adapterInfo : adapterInfos)
	{
		if (firstAdapterIdx == -1)
		{
			firstAdapterIdx = adapterIdx;
		}

		if (adapterInfo.adapterType == AdapterType::Discrete && firstDiscreteAdapterIdx == -1)
		{
			firstDiscreteAdapterIdx = adapterIdx;
		}

		if (adapterInfo.adapterType == AdapterType::Software && warpAdapterIdx == -1 && m_desc.allowSoftwareDevice)
		{
			warpAdapterIdx = adapterIdx;
		}

		if (adapterInfo.dedicatedVideoMemory > maxMemory)
		{
			maxMemory = adapterInfo.dedicatedVideoMemory;
			bestMemoryAdapterIdx = adapterIdx;
		}

		++adapterIdx;
	}

	// Now choose our best adapter
	if (m_desc.preferDiscreteDevice)
	{
		if (bestMemoryAdapterIdx != -1)
		{
			chosenAdapterIdx = bestMemoryAdapterIdx;
		}
		else if (firstDiscreteAdapterIdx != -1)
		{
			chosenAdapterIdx = firstDiscreteAdapterIdx;
		}
		else
		{
			chosenAdapterIdx = firstAdapterIdx;
		}
	}
	else
	{
		chosenAdapterIdx = firstAdapterIdx;
	}

	if (chosenAdapterIdx == -1)
	{
		LogFatal(LogDirectX) << "Failed to select a D3D12 adapter." << endl;
		return;
	}

	// Create device, either WARP or hardware
	wil::com_ptr<ID3D12Device> device;
	if (chosenAdapterIdx == warpAdapterIdx)
	{
		assert_succeeded(m_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&m_dxgiAdapter)));
		assert_succeeded(D3D12CreateDevice(m_dxgiAdapter.get(), m_bestFeatureLevel, IID_PPV_ARGS(&device)));

		m_bIsWarpAdapter = true;

		LogWarning(LogDirectX) << "Failed to find a hardware adapter, falling back to WARP." << endl;
	}
	else
	{
		assert_succeeded(m_dxgiFactory->EnumAdapters((UINT)chosenAdapterIdx, &m_dxgiAdapter));
		assert_succeeded(D3D12CreateDevice(m_dxgiAdapter.get(), m_bestFeatureLevel, IID_PPV_ARGS(&device)));

		LogInfo(LogDirectX) << "Selected D3D12 adapter " << chosenAdapterIdx << endl;
	}

	m_bIsAgilitySDKAvailable = IsDirectXAgilitySDKAvailable();
	LogInfo(LogDirectX) << "Agility SDK " << (m_bIsAgilitySDKAvailable ? "is" : "is not") << " available." << endl;

#ifndef _RELEASE
	if (!m_bIsWarpAdapter)
	{
		// Prevent the GPU from overclocking or underclocking to get consistent timings
		if (m_bIsDeveloperModeEnabled && !m_bIsRenderDocAvailable)
		{
			device->SetStablePowerState(TRUE);
		}
	}
#endif

	// Create D3D12 Memory Allocator
	wil::com_ptr<D3D12MA::Allocator> d3d12maAllocator;
	auto allocatorDesc = D3D12MA::ALLOCATOR_DESC{
		.Flags					= D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED | D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED,
		.pDevice				= device.get(),
		.PreferredBlockSize		= 0,
		.pAllocationCallbacks	= nullptr,
		.pAdapter				= m_dxgiAdapter.get()
	};
	assert_succeeded(D3D12MA::CreateAllocator(&allocatorDesc, &d3d12maAllocator));


	// Create Luna GraphicsDevice
	auto deviceDesc = GraphicsDeviceDesc{
		.dxgiFactory				= m_dxgiFactory.get(),
		.dx12Device					= device.get(),
		.d3d12maAllocator			= d3d12maAllocator.get(),
		.backBufferWidth			= m_desc.backBufferWidth,
		.backBufferHeight			= m_desc.backBufferHeight,
		.numSwapChainBuffers		= m_desc.numSwapChainBuffers,
		.swapChainFormat			= m_desc.swapChainFormat,
		.swapChainSampleCount		= m_desc.swapChainSampleCount,
		.swapChainSampleQuality		= m_desc.swapChainSampleQuality,
		.allowModeSwitch			= m_desc.allowModeSwitch,
		.isTearingSupported			= m_bIsTearingSupported,
		.enableVSync				= m_desc.enableVSync,
		.maxFramesInFlight			= m_desc.maxFramesInFlight,
		.hwnd						= m_desc.hwnd,
		.enableValidation			= m_desc.enableValidation,
		.enableDebugMarkers			= m_desc.enableDebugMarkers
	};

	m_device = Make<GraphicsDevice>(deviceDesc);

	m_device->CreateResources();
}


vector<AdapterInfo> DeviceManager::EnumerateAdapters()
{
	vector<AdapterInfo> adapters;

	wil::com_ptr<IDXGIFactory6> dxgiFactory6;
	m_dxgiFactory->QueryInterface(IID_PPV_ARGS(&dxgiFactory6));

	const D3D_FEATURE_LEVEL minRequiredLevel{ D3D_FEATURE_LEVEL_11_0 };
	const DXGI_GPU_PREFERENCE gpuPreference{ DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE };

	IDXGIAdapter* tempAdapter{ nullptr };

	LogInfo(LogDirectX) << "Enumerating DXGI adapters..." << endl;

	for (int32_t idx = 0; DXGI_ERROR_NOT_FOUND != EnumAdapter((UINT)idx, gpuPreference, dxgiFactory6.get(), &tempAdapter); ++idx)
	{
		DXGI_ADAPTER_DESC desc{};
		tempAdapter->GetDesc(&desc);

		DeviceBasicCaps basicCaps{};

		if (TestCreateDevice(tempAdapter, minRequiredLevel, basicCaps))
		{
			AdapterInfo adapterInfo{};

			adapterInfo.name = MakeStr(desc.Description);
			adapterInfo.deviceId = desc.DeviceId;
			adapterInfo.vendorId = desc.VendorId;
			adapterInfo.dedicatedVideoMemory = desc.DedicatedVideoMemory;
			adapterInfo.dedicatedSystemMemory = desc.DedicatedSystemMemory;
			adapterInfo.sharedSystemMemory = desc.SharedSystemMemory;
			adapterInfo.vendor = VendorIdToHardwareVendor(adapterInfo.vendorId);
			adapterInfo.adapterType = GetAdapterType(tempAdapter);

			LogInfo(LogDirectX) << format("  Adapter {} is D3D12-capable: {} (Vendor: {}, VendorId: {:#x}, DeviceId: {:#x})",
				idx,
				adapterInfo.name,
				HardwareVendorToString(adapterInfo.vendor),
				adapterInfo.vendorId, adapterInfo.deviceId)
				<< endl;

			LogInfo(LogDirectX) << format("    Feature level {}, shader model {}, binding tier {}, wave ops {}, atomic64 {}",
				D3DTypeToString(basicCaps.maxFeatureLevel, true),
				D3DTypeToString(basicCaps.maxShaderModel, true),
				D3DTypeToString(basicCaps.resourceBindingTier, true),
				basicCaps.bSupportsWaveOps ? "supported" : "not supported",
				basicCaps.bSupportsAtomic64 ? "supported" : "not supported")
				<< endl;

			LogInfo(LogDirectX) << format("    Adapter memory: {} MB dedicated video memory, {} MB dedicated system memory, {} MB shared memory",
				(uint32_t)(desc.DedicatedVideoMemory >> 20),
				(uint32_t)(desc.DedicatedSystemMemory >> 20),
				(uint32_t)(desc.SharedSystemMemory >> 20))
				<< endl;

			m_bestFeatureLevel = basicCaps.maxFeatureLevel;
			m_bestShaderModel = basicCaps.maxShaderModel;

			adapters.push_back(adapterInfo);
		}

		tempAdapter->Release();
		tempAdapter = nullptr;
	}

	return adapters;
}


HRESULT DeviceManager::EnumAdapter(int32_t adapterIdx, DXGI_GPU_PREFERENCE gpuPreference, IDXGIFactory6* dxgiFactory6, IDXGIAdapter** adapter)
{
	if (!dxgiFactory6 || gpuPreference == DXGI_GPU_PREFERENCE_UNSPECIFIED)
	{
		return m_dxgiFactory->EnumAdapters((UINT)adapterIdx, adapter);
	}
	else
	{
		return dxgiFactory6->EnumAdapterByGpuPreference((UINT)adapterIdx, gpuPreference, IID_PPV_ARGS(adapter));
	}
}


Queue& DeviceManager::GetQueue(QueueType queueType)
{
	return *m_queues[(uint32_t)queueType];
}


Queue& DeviceManager::GetQueue(CommandListType commandListType)
{
	const auto queueType = CommandListTypeToQueueType(commandListType);
	return GetQueue(queueType);
}


void DeviceManager::UpdateColorSpace()
{
	if (!m_dxgiFactory)
		return;

	if (!m_dxgiFactory->IsCurrent())
	{
		// Output information is cached on the DXGI Factory. If it is stale we need to create a new factory.
		ThrowIfFailed(CreateDXGIFactory2(m_desc.enableValidation ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(m_dxgiFactory.put())));
	}

	DXGI_COLOR_SPACE_TYPE colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;

	bool isDisplayHDR10 = false;

	if (m_dxSwapChain)
	{
		// To detect HDR support, we will need to check the color space in the primary
		// DXGI output associated with the app at this point in time
		// (using window/display intersection).

		// Get the rectangle bounds of the app window.
		RECT windowBounds;
		if (!GetWindowRect(m_desc.hwnd, &windowBounds))
			throw std::system_error(std::error_code(static_cast<int>(GetLastError()), std::system_category()), "GetWindowRect");

		const long ax1 = windowBounds.left;
		const long ay1 = windowBounds.top;
		const long ax2 = windowBounds.right;
		const long ay2 = windowBounds.bottom;

		wil::com_ptr<IDXGIOutput> bestOutput;
		long bestIntersectArea = -1;

		wil::com_ptr<IDXGIAdapter> adapter;
		for (UINT adapterIndex = 0;
			SUCCEEDED(m_dxgiFactory->EnumAdapters(adapterIndex, adapter.put()));
			++adapterIndex)
		{
			wil::com_ptr<IDXGIOutput> output;
			for (UINT outputIndex = 0;
				SUCCEEDED(adapter->EnumOutputs(outputIndex, output.put()));
				++outputIndex)
			{
				// Get the rectangle bounds of current output.
				DXGI_OUTPUT_DESC desc;
				ThrowIfFailed(output->GetDesc(&desc));
				const auto& r = desc.DesktopCoordinates;

				// Compute the intersection
				const long intersectArea = ComputeIntersectionArea(ax1, ay1, ax2, ay2, r.left, r.top, r.right, r.bottom);
				if (intersectArea > bestIntersectArea)
				{
					bestOutput.swap(output);
					bestIntersectArea = intersectArea;
				}
			}
		}

		if (bestOutput)
		{
			auto output6 = bestOutput.query<IDXGIOutput6>();
			if (output6)
			{
				DXGI_OUTPUT_DESC1 desc;
				ThrowIfFailed(output6->GetDesc1(&desc));

				if (desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
				{
					// Display output is HDR10.
					isDisplayHDR10 = true;
				}
			}
		}
	}

	if (m_desc.allowHDROutput && isDisplayHDR10)
	{
		switch (m_desc.swapChainFormat)
		{
		case Format::R10G10B10A2_UNorm:
			// The application creates the HDR10 signal.
			colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
			break;

		case Format::RGBA16_Float:
			// The system creates the HDR10 signal; application uses linear values.
			colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
			break;

		default:
			break;
		}
	}

	m_colorSpace = colorSpace;

	UINT colorSpaceSupport = 0;
	if (m_dxSwapChain
		&& SUCCEEDED(m_dxSwapChain->CheckColorSpaceSupport(colorSpace, &colorSpaceSupport))
		&& (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
	{
		ThrowIfFailed(m_dxSwapChain->SetColorSpace1(colorSpace));
	}
}


void DeviceManager::ResizeSwapChain()
{
	ReleaseSwapChainBuffers();

	CreateWindowSizeDependentResources();
}


void DeviceManager::ReleaseSwapChainBuffers()
{
	WaitForGpu();

	m_swapChainBuffers.clear();
}


void DeviceManager::ReleaseDeferredResources()
{
	auto resourceIt = m_deferredResources.begin();
	while (resourceIt != m_deferredResources.end())
	{
		if (GetQueue(QueueType::Graphics).IsFenceComplete(resourceIt->fenceValue))
		{
			resourceIt = m_deferredResources.erase(resourceIt);
		}
		else
		{
			++resourceIt;
		}
	}
}


DeviceManager* GetD3D12DeviceManager()
{
	return g_d3d12DeviceManager;
}

} // namespace Luna::DX12