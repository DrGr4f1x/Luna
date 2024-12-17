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

#include "Device12.h"

#include "DescriptorAllocator12.h"
#include "DeviceCaps12.h"
#include "Formats12.h"
#include "Queue12.h"

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


DeviceRLDOHelper::~DeviceRLDOHelper()
{
	if (device && doReport)
	{
		ComPtr<ID3D12DebugDevice> debugInterface;
		if (SUCCEEDED(device->QueryInterface(debugInterface.GetAddressOf())))
		{
			debugInterface->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
		}
	}
}


GraphicsDevice::GraphicsDevice(const GraphicsDeviceDesc& desc) noexcept
	: m_desc{ desc }
	, m_deviceRLDOHelper{ desc.dx12Device, desc.enableValidation }
{
	LogInfo(LogDirectX) << "Creating DirectX 12 device." << endl;

	m_dxgiFactory = m_desc.dxgiFactory;
	m_dxDevice = m_desc.dx12Device;

	SetDebugName(m_dxDevice.Get(), "DX12 Device");
}


GraphicsDevice::~GraphicsDevice()
{
	LogInfo(LogDirectX) << "Destroying DirectX 12 device." << endl;

	if (m_dxInfoQueue)
	{
		m_dxInfoQueue->UnregisterMessageCallback(m_callbackCookie);
		m_dxInfoQueue.Reset();
	}
}


void GraphicsDevice::WaitForGpu()
{
	if (m_bQueuesCreated)
	{
		m_queues[(uint32_t)QueueType::Graphics]->WaitForGpu();
		m_queues[(uint32_t)QueueType::Compute]->WaitForGpu();
		m_queues[(uint32_t)QueueType::Copy]->WaitForGpu();
	}
}


void GraphicsDevice::CreateResources()
{
	if (m_desc.enableValidation)
	{
		InstallDebugCallback();
	}

	// Create queues
	m_queues[(uint32_t)QueueType::Graphics] = make_unique<Queue>(m_dxDevice.Get(), QueueType::Graphics);
	m_queues[(uint32_t)QueueType::Compute] = make_unique<Queue>(m_dxDevice.Get(), QueueType::Compute);
	m_queues[(uint32_t)QueueType::Copy] = make_unique<Queue>(m_dxDevice.Get(), QueueType::Copy);
	m_bQueuesCreated = true;

	// Create descriptor allocators
	m_descriptorAllocators[0] = make_unique<DescriptorAllocator>(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_descriptorAllocators[1] = make_unique<DescriptorAllocator>(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	m_descriptorAllocators[2] = make_unique<DescriptorAllocator>(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_descriptorAllocators[3] = make_unique<DescriptorAllocator>(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	m_caps = make_unique<DeviceCaps>();
	ReadCaps();
}


void GraphicsDevice::CreateWindowSizeDependentResources()
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
			(m_desc.isTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u));

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
		swapChainDesc.Flags = m_desc.isTearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u;

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
	for (UINT n = 0; n < backBufferCount; n++)
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
	}

	// Reset the index to the current back buffer.
	m_backBufferIndex = m_dxSwapChain->GetCurrentBackBufferIndex();
}


void GraphicsDevice::InstallDebugCallback()
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


void GraphicsDevice::ReadCaps()
{
	const D3D_FEATURE_LEVEL minFeatureLevel{ D3D_FEATURE_LEVEL_12_0 };
	const D3D_SHADER_MODEL maxShaderModel{ D3D_SHADER_MODEL_6_8 };

	m_caps->ReadFullCaps(m_dxDevice.Get(), minFeatureLevel, maxShaderModel);

	// TODO
	//if (g_graphicsDeviceOptions.logDeviceFeatures)
	if (false)
	{
		m_caps->LogCaps();
	}
}


D3D12_CPU_DESCRIPTOR_HANDLE GraphicsDevice::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t count)
{
	return m_descriptorAllocators[type]->Allocate(m_dxDevice.Get(), count);
}


Queue& GraphicsDevice::GetQueue(QueueType queueType)
{
	return *m_queues[(uint32_t)queueType];
}


Queue& GraphicsDevice::GetQueue(CommandListType commandListType)
{
	const auto queueType = CommandListTypeToQueueType(commandListType);
	return GetQueue(queueType);
}

void GraphicsDevice::HandleDeviceLost()
{
	// TODO
}


void GraphicsDevice::UpdateColorSpace()
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