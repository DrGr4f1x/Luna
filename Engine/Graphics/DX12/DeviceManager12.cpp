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
#include "Graphics\DX12\DeviceCaps12.h"
#include "Graphics\DX12\Device12.h"
#include "Graphics\DX12\Queue12.h"

using namespace std;
using namespace Microsoft::WRL;


namespace Luna::DX12
{

bool IsDirectXAgilitySDKAvailable()
{
	HMODULE agilitySDKDllHandle = ::GetModuleHandle("D3D12Core.dll");
	return agilitySDKDllHandle != nullptr;
}


bool IsAdapterIntegrated(IDXGIAdapter* adapter)
{
	IntrusivePtr<IDXGIAdapter3> adapter3;
	adapter->QueryInterface(IID_PPV_ARGS(&adapter3));

	DXGI_QUERY_VIDEO_MEMORY_INFO nonLocalVideoMemoryInfo{};
	if (adapter3 && SUCCEEDED(adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &nonLocalVideoMemoryInfo)))
	{
		return nonLocalVideoMemoryInfo.Budget == 0;
	}

	return true;
}


bool TestCreateDevice(IDXGIAdapter* adapter, D3D_FEATURE_LEVEL minFeatureLevel, DeviceBasicCaps& deviceBasicCaps)
{
	ComPtr<ID3D12Device> device;
	if (SUCCEEDED(D3D12CreateDevice(adapter, minFeatureLevel, IID_PPV_ARGS(&device))))
	{
		DeviceCaps testCaps;
		testCaps.ReadBasicCaps(device.Get(), minFeatureLevel);
		deviceBasicCaps = testCaps.basicCaps;

		return true;
	}
	return false;
}


AdapterType GetAdapterType(IDXGIAdapter* adapter)
{
	ComPtr<IDXGIAdapter3> adapter3;
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
		ComPtr<IDXGIDebug1> pDebug;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
		{
			pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		}
	}
}


DeviceManager::DeviceManager(const DeviceManagerDesc& desc)
	: m_desc{ desc }
{
	m_bIsDeveloperModeEnabled = IsDeveloperModeEnabled();
	m_bIsRenderDocAvailable = IsRenderDocAvailable();
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


void DeviceManager::CreateDeviceResources()
{
	m_dxgiRLOHelper.doReport = m_desc.enableValidation;

	if (m_desc.enableValidation)
	{
		ComPtr<ID3D12Debug> debugInterface;
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
	SetDebugName(m_dxgiFactory.Get(), "DXGI Factory");

	ComPtr<IDXGIFactory5> dxgiFactory5;
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
	m_queues[(uint32_t)QueueType::Graphics] = make_unique<Queue>(m_device->GetDevice(), QueueType::Graphics);
	m_queues[(uint32_t)QueueType::Compute] = make_unique<Queue>(m_device->GetDevice(), QueueType::Compute);
	m_queues[(uint32_t)QueueType::Copy] = make_unique<Queue>(m_device->GetDevice(), QueueType::Copy);
	m_bQueuesCreated = true;
}


void DeviceManager::CreateWindowSizeDependentResources()
{ 
	assert(m_desc.hwnd);

	WaitForGpu();

	// Release resources tied to swap chain and update fence values
	// TODO: hard-coded backbuffer count is 3 here.  Get this from somewhere else.
	const uint32_t backBufferCount{ 3 };
	for (uint32_t i = 0; i < backBufferCount; ++i)
	{
		m_renderTargets[i].Reset();
		m_fenceValues[i] = m_fenceValues[m_backBufferIndex];
	}

	// TODO: The window dimensions might have changed externally.  Need to pass those in from somewhere else (DeviceManager?).
	const uint32_t newBackBufferWidth = m_desc.backBufferWidth;
	const uint32_t newBackBufferHeight = m_desc.backBufferHeight;
	DXGI_FORMAT dxgiFormat = FormatToDxgi(RemoveSrgb(m_desc.swapChainFormat)).resourceFormat;

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
				static_cast<uint32_t>((hr == DXGI_ERROR_DEVICE_REMOVED) ? m_device->GetDevice()->GetDeviceRemovedReason() : hr)) << endl;

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
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
		swapChainDesc.Width = newBackBufferWidth;
		swapChainDesc.Height = newBackBufferHeight;
		swapChainDesc.Format = dxgiFormat;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = backBufferCount;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		swapChainDesc.Flags = m_bIsTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc{};
		fsSwapChainDesc.Windowed = TRUE;

		// Create a swap chain for the window.
		ComPtr<IDXGISwapChain1> swapChain;
		ThrowIfFailed(m_dxgiFactory->CreateSwapChainForHwnd(
			GetQueue(QueueType::Graphics).GetCommandQueue(),
			m_desc.hwnd,
			&swapChainDesc,
			&fsSwapChainDesc,
			nullptr,
			swapChain.GetAddressOf()
		));

		ThrowIfFailed(swapChain.As(&m_dxSwapChain));

		// This class does not support exclusive full-screen mode and prevents DXGI from responding to the ALT+ENTER shortcut
		ThrowIfFailed(m_dxgiFactory->MakeWindowAssociation(m_desc.hwnd, DXGI_MWA_NO_ALT_ENTER));
	}

	// Handle color space settings for HDR
	UpdateColorSpace();

	// Obtain the back buffers for this window which will be the final render targets
   // and create render target views for each of them.
	// TODO: replace this with CreateColorBufferFromSwapChain
	/*for (UINT n = 0; n < backBufferCount; n++)
	{
		ThrowIfFailed(m_dxSwapChain->GetBuffer(n, IID_PPV_ARGS(m_renderTargets[n].GetAddressOf())));

		wchar_t name[25] = {};
		swprintf_s(name, L"Render target %u", n);
		m_renderTargets[n]->SetName(name);

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = dxgiFormat;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

		const auto rtvDescriptor = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_dxDevice->CreateRenderTargetView(m_renderTargets[n].Get(), &rtvDesc, rtvDescriptor);
	}*/

	// Reset the index to the current back buffer.
	m_backBufferIndex = m_dxSwapChain->GetCurrentBackBufferIndex();
}


void DeviceManager::HandleDeviceLost()
{ }


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
	ComPtr<ID3D12Device> device;
	if (chosenAdapterIdx == warpAdapterIdx)
	{
		ComPtr<IDXGIAdapter> tempAdapter;
		assert_succeeded(m_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&tempAdapter)));
		assert_succeeded(D3D12CreateDevice(tempAdapter.Get(), m_bestFeatureLevel, IID_PPV_ARGS(&device)));

		m_bIsWarpAdapter = true;

		LogWarning(LogDirectX) << "Failed to find a hardware adapter, falling back to WARP." << endl;
	}
	else
	{
		ComPtr<IDXGIAdapter> tempAdapter;
		assert_succeeded(m_dxgiFactory->EnumAdapters((UINT)chosenAdapterIdx, &tempAdapter));
		assert_succeeded(D3D12CreateDevice(tempAdapter.Get(), m_bestFeatureLevel, IID_PPV_ARGS(&device)));

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

	// Create Luna GraphicsDevice
	auto deviceDesc = GraphicsDeviceDesc{}
		.SetDxgiFactory(m_dxgiFactory.Get())
		.SetDevice(device.Get())
		.SetBackBufferWidth(m_desc.backBufferWidth)
		.SetBackBufferHeight(m_desc.backBufferHeight)
		.SetNumSwapChainBuffers(m_desc.numSwapChainBuffers)
		.SetSwapChainFormat(m_desc.swapChainFormat)
		.SetSwapChainSampleCount(m_desc.swapChainSampleCount)
		.SetSwapChainSampleQuality(m_desc.swapChainSampleQuality)
		.SetAllowModeSwitch(m_desc.allowModeSwitch)
		.SetIsTearingSupported(m_bIsTearingSupported)
		.SetEnableVSync(m_desc.enableVSync)
		.SetMaxFramesInFlight(m_desc.maxFramesInFlight)
		.SetHwnd(m_desc.hwnd)
		.SetEnableValidation(m_desc.enableValidation)
		.SetEnableDebugMarkers(m_desc.enableDebugMarkers);

	m_device = Make<GraphicsDevice>(deviceDesc);

	m_device->CreateResources();
}


vector<AdapterInfo> DeviceManager::EnumerateAdapters()
{
	vector<AdapterInfo> adapters;

	ComPtr<IDXGIFactory6> dxgiFactory6;
	m_dxgiFactory->QueryInterface(IID_PPV_ARGS(&dxgiFactory6));

	const D3D_FEATURE_LEVEL minRequiredLevel{ D3D_FEATURE_LEVEL_11_0 };
	const DXGI_GPU_PREFERENCE gpuPreference{ DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE };

	IDXGIAdapter* tempAdapter{ nullptr };

	LogInfo(LogDirectX) << "Enumerating DXGI adapters..." << endl;

	for (int32_t idx = 0; DXGI_ERROR_NOT_FOUND != EnumAdapter((UINT)idx, gpuPreference, dxgiFactory6.Get(), &tempAdapter); ++idx)
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
		ThrowIfFailed(CreateDXGIFactory2(m_desc.enableValidation ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(m_dxgiFactory.ReleaseAndGetAddressOf())));
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

		ComPtr<IDXGIOutput> bestOutput;
		long bestIntersectArea = -1;

		ComPtr<IDXGIAdapter> adapter;
		for (UINT adapterIndex = 0;
			SUCCEEDED(m_dxgiFactory->EnumAdapters(adapterIndex, adapter.ReleaseAndGetAddressOf()));
			++adapterIndex)
		{
			ComPtr<IDXGIOutput> output;
			for (UINT outputIndex = 0;
				SUCCEEDED(adapter->EnumOutputs(outputIndex, output.ReleaseAndGetAddressOf()));
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
					bestOutput.Swap(output);
					bestIntersectArea = intersectArea;
				}
			}
		}

		if (bestOutput)
		{
			ComPtr<IDXGIOutput6> output6;
			if (SUCCEEDED(bestOutput.As(&output6)))
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

} // namespace Luna::DX12