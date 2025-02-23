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

#include "ColorBufferManager12.h"
#include "CommandContext12.h"
#include "DepthBufferManager12.h"
#include "DescriptorSetPool12.h"
#include "DeviceCaps12.h"
#include "GpuBufferPool12.h"
#include "PipelineStatePool12.h"
#include "Queue12.h"
#include "RootSignaturePool12.h"
#include "Shader12.h"

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


void DebugMessageCallback(
	D3D12_MESSAGE_CATEGORY category,
	D3D12_MESSAGE_SEVERITY severity,
	D3D12_MESSAGE_ID id,
	LPCSTR pDescription,
	void* pContext)
{
	string debugMessage = format("[{}] Code {} : {}", category, (uint32_t)id, pDescription);

	switch (severity)
	{
	case D3D12_MESSAGE_SEVERITY_CORRUPTION:
		LogFatal(LogDirectX) << debugMessage << endl;
		break;
	case D3D12_MESSAGE_SEVERITY_ERROR:
		LogError(LogDirectX) << debugMessage << endl;
		break;
	case D3D12_MESSAGE_SEVERITY_WARNING:
		LogWarning(LogDirectX) << debugMessage << endl;
		break;
	case D3D12_MESSAGE_SEVERITY_INFO:
	case D3D12_MESSAGE_SEVERITY_MESSAGE:
		LogInfo(LogDirectX) << debugMessage << endl;
		break;
	}
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


DeviceRLDOHelper::~DeviceRLDOHelper()
{
	if (device && doReport)
	{
		wil::com_ptr<ID3D12DebugDevice> debugInterface;
		if (SUCCEEDED(device->QueryInterface(debugInterface.addressof())))
		{
			debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
		}
	}
}


DeviceManager::DeviceManager(const DeviceManagerDesc& desc)
	: m_desc{ desc }
	, m_deviceRLDOHelper{ desc.enableValidation }
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

	Shader::DestroyAll();
	g_userDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Destroy();
	g_userDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER].Destroy();

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


bool DeviceManager::IsFenceComplete(uint64_t fenceValue)
{
	return GetQueue(static_cast<CommandListType>(fenceValue >> 56)).IsFenceComplete(fenceValue);
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
	
	CreateResourceManagers();

	// Create queues
	m_queues[(uint32_t)QueueType::Graphics] = make_unique<Queue>(m_dxDevice.get(), QueueType::Graphics);
	m_queues[(uint32_t)QueueType::Compute] = make_unique<Queue>(m_dxDevice.get(), QueueType::Compute);
	m_queues[(uint32_t)QueueType::Copy] = make_unique<Queue>(m_dxDevice.get(), QueueType::Copy);
	m_bQueuesCreated = true;

	// Create a fence for tracking GPU execution progress
	ThrowIfFailed(m_dxDevice->CreateFence(m_fenceValues[m_backBufferIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));

	m_fenceValues[m_backBufferIndex]++;

	m_fence->SetName(L"DeviceResources");

	m_fenceEvent.Attach(CreateEventEx(nullptr, nullptr, 0, EVENT_MODIFY_STATE | SYNCHRONIZE));

	if (m_desc.enableValidation)
	{
		InstallDebugCallback();
	}

	// Create descriptor allocators
	m_descriptorAllocators[0] = make_unique<DescriptorAllocator>(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_descriptorAllocators[1] = make_unique<DescriptorAllocator>(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	m_descriptorAllocators[2] = make_unique<DescriptorAllocator>(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_descriptorAllocators[3] = make_unique<DescriptorAllocator>(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	g_userDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Create("User Descriptor Heap, CBV_SRV_UAV");
	g_userDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER].Create("User Descriptor Heap, SAMPLER");

	m_caps = make_unique<DeviceCaps>();
	ReadCaps();
	m_caps->LogCaps();
}


void DeviceManager::CreateWindowSizeDependentResources()
{ 
	assert(m_desc.hwnd);

	WaitForGpu();

	// Release resources tied to swap chain and update fence values
	m_swapChainBuffers.clear();
	const uint32_t backBufferCount = m_desc.numSwapChainBuffers;

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
				static_cast<uint32_t>((hr == DXGI_ERROR_DEVICE_REMOVED) ? m_dxDevice->GetDeviceRemovedReason() : hr)) << endl;

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
		ColorBuffer swapChainBuffer;
		swapChainBuffer.SetHandle(GetD3D12ColorBufferManager()->CreateColorBufferFromSwapChain(m_dxSwapChain.get(), i).get());
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

	assert_succeeded(m_dxDevice->CreateCommandList(1, CommandListTypeToDX12(commandListType), *allocator, nullptr, IID_PPV_ARGS(commandList)));

	(*commandList)->SetName(L"CommandList");
}


void DeviceManager::FreeContext(CommandContext* usedContext)
{
	assert(usedContext != nullptr);
	lock_guard<mutex> lockGuard(m_contextAllocationMutex);

	m_availableContexts[(uint32_t)usedContext->GetType()].push(usedContext);
}


ColorBuffer& DeviceManager::GetColorBuffer()
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


IColorBufferManager* DeviceManager::GetColorBufferManager()
{
	return m_colorBufferManager.get();
}


IDepthBufferManager* DeviceManager::GetDepthBufferManager()
{
	return m_depthBufferManager.get();
}


IDescriptorSetPool* DeviceManager::GetDescriptorSetPool()
{
	return m_descriptorSetPool.get();
}


IGpuBufferPool* DeviceManager::GetGpuBufferPool()
{
	return m_gpuBufferPool.get();
}


IPipelineStatePool* DeviceManager::GetPipelineStatePool()
{
	return m_pipelineStatePool.get();
}


IRootSignaturePool* DeviceManager::GetRootSignaturePool()
{
	return m_rootSignaturePool.get();
}


uint8_t DeviceManager::GetFormatPlaneCount(DXGI_FORMAT format)
{
	uint8_t& planeCount = m_dxgiFormatPlaneCounts[format];
	if (planeCount == 0)
	{
		D3D12_FEATURE_DATA_FORMAT_INFO formatInfo{ format, 1 };
		if (FAILED(m_dxDevice->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO, &formatInfo, sizeof(formatInfo))))
		{
			// Format not supported, store a special value in the cache to avoid querying later
			planeCount = 255;
		}
		else
		{
			// Format supported - store the plane count in the cache
			planeCount = formatInfo.PlaneCount;
		}
	}

	if (planeCount == 255)
	{
		return 0;
	}

	return planeCount;
}


D3D12_CPU_DESCRIPTOR_HANDLE DeviceManager::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count)
{
	return m_descriptorAllocators[type]->Allocate(m_dxDevice.get(), count);
}


void DeviceManager::HandleDeviceLost()
{ 
	// TODO
	/*if (m_deviceNotify)
	{
		m_deviceNotify->OnDeviceLost();
	}*/

	m_swapChainBuffers.clear();

	m_queues[(uint32_t)QueueType::Graphics].reset();
	m_queues[(uint32_t)QueueType::Compute].reset();
	m_queues[(uint32_t)QueueType::Copy].reset();

	m_dxDevice.reset();

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

	m_dxDevice = device;

	m_deviceRLDOHelper.device = device.get();

	// Create D3D12 Memory Allocator
	auto allocatorDesc = D3D12MA::ALLOCATOR_DESC{
		.Flags					= D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED | D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED,
		.pDevice				= device.get(),
		.PreferredBlockSize		= 0,
		.pAllocationCallbacks	= nullptr,
		.pAdapter				= m_dxgiAdapter.get()
	};
	assert_succeeded(D3D12MA::CreateAllocator(&allocatorDesc, &m_d3d12maAllocator));

	// TODO: Create descriptor allocators
}


void DeviceManager::CreateResourceManagers()
{
	m_colorBufferManager = make_unique<ColorBufferManager>(m_dxDevice.get(), m_d3d12maAllocator.get());
	m_depthBufferManager = make_unique<DepthBufferManager>(m_dxDevice.get(), m_d3d12maAllocator.get());
	m_descriptorSetPool = make_unique<DescriptorSetPool>(m_dxDevice.get());
	m_gpuBufferPool = make_unique<GpuBufferPool>(m_dxDevice.get(), m_d3d12maAllocator.get());
	m_pipelineStatePool = make_unique<PipelineStatePool>(m_dxDevice.get());
	m_rootSignaturePool = make_unique<RootSignaturePool>(m_dxDevice.get());
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


void DeviceManager::InstallDebugCallback()
{
	if (SUCCEEDED(m_dxDevice->QueryInterface(IID_PPV_ARGS(&m_dxInfoQueue))))
	{
		// Suppress whole categories of messages
		//D3D12_MESSAGE_CATEGORY Categories[] = {};

		// Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID denyIds[] =
		{
			// This occurs when there are uninitialized descriptors in a descriptor table, even when a
			// shader does not access the missing descriptors.  I find this is common when switching
			// shader permutations and not wanting to change much code to reorder resources.
			D3D12_MESSAGE_ID_INVALID_DESCRIPTOR_HANDLE,

			// Triggered when a shader does not export all color components of a render target, such as
			// when only writing RGB to an R10G10B10A2 buffer, ignoring alpha.
			D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_PS_OUTPUT_RT_OUTPUT_MISMATCH,

			// This occurs when a descriptor table is unbound even when a shader does not access the missing
			// descriptors.  This is common with a root signature shared between disparate shaders that
			// don't all need the same types of resources.
			D3D12_MESSAGE_ID_COMMAND_LIST_DESCRIPTOR_TABLE_NOT_SET,

			D3D12_MESSAGE_ID_COPY_DESCRIPTORS_INVALID_RANGES,

			// Silence complaints about shaders not being signed by DXIL.dll.  We don't care about this.
			D3D12_MESSAGE_ID_NON_RETAIL_SHADER_MODEL_WONT_VALIDATE,

			// RESOURCE_BARRIER_DUPLICATE_SUBRESOURCE_TRANSITIONS
			(D3D12_MESSAGE_ID)1008,
		};

		D3D12_INFO_QUEUE_FILTER newFilter = {};
		//newFilter.DenyList.NumCategories = _countof(Categories);
		//newFilter.DenyList.pCategoryList = Categories;
		newFilter.DenyList.NumSeverities = _countof(severities);
		newFilter.DenyList.pSeverityList = severities;
		newFilter.DenyList.NumIDs = _countof(denyIds);
		newFilter.DenyList.pIDList = denyIds;

#ifdef _DEBUG
		m_dxInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		m_dxInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
#endif
		m_dxInfoQueue->PushStorageFilter(&newFilter);
		m_dxInfoQueue->RegisterMessageCallback(DebugMessageCallback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &m_callbackCookie);
	}
}


void DeviceManager::ReadCaps()
{
	const D3D_FEATURE_LEVEL minFeatureLevel{ D3D_FEATURE_LEVEL_12_0 };
	const D3D_SHADER_MODEL maxShaderModel{ D3D_SHADER_MODEL_6_8 };

	m_caps->ReadFullCaps(m_dxDevice.get(), minFeatureLevel, maxShaderModel);

	// TODO
	//if (g_graphicsDeviceOptions.logDeviceFeatures)
	if (false)
	{
		m_caps->LogCaps();
	}
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


D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count)
{
	return GetD3D12DeviceManager()->AllocateDescriptor(type, count);
}


uint8_t GetFormatPlaneCount(DXGI_FORMAT format)
{
	return GetD3D12DeviceManager()->GetFormatPlaneCount(format);
}


uint32_t GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
	return GetD3D12DeviceManager()->GetDevice()->GetDescriptorHandleIncrementSize(type);
}

} // namespace Luna::DX12