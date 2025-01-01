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

#include "DeviceManagerVK.h"

#include "Graphics\GraphicsCommon.h"
#include "Graphics\Vulkan\ColorBufferVK.h"
#include "Graphics\Vulkan\CommandContextVK.h"
#include "Graphics\Vulkan\DeviceVK.h"
#include "Graphics\Vulkan\QueueVK.h"

using namespace std;
using namespace Microsoft::WRL;


namespace Luna::VK
{

DeviceManager* g_vulkanDeviceManager{ nullptr };


size_t GetDedicatedVideoMemory(VkPhysicalDevice physicalDevice)
{
	size_t memory{ 0 };

	VkPhysicalDeviceMemoryProperties memoryProperties{};
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	for (const auto& heap : memoryProperties.memoryHeaps)
	{
		if (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
		{
			memory += heap.size;
		}
	}

	return memory;
}


VkBool32 DebugMessageCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	string prefix;

	if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
	{
		prefix += prefix.empty() ? "[Performance]" : " [Performance]";
	}
	if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
	{
		prefix += prefix.empty() ? "[Validation]" : " [Validation]";
	}
	if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
	{
		prefix += prefix.empty() ? "[General]" : " [General]";
	}

	string debugMessage = format("{} [{}] Code {} : {}", prefix, pCallbackData->pMessageIdName, pCallbackData->messageIdNumber, pCallbackData->pMessage);

	if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		LogError(LogVulkan) << debugMessage << endl;
	}
	else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		LogWarning(LogVulkan) << debugMessage << endl;
	}
	else
	{
		LogInfo(LogVulkan) << debugMessage << endl;
	}

	// The return value of this callback controls whether the Vulkan call that caused
	// the validation message will be aborted or not
	// We return VK_FALSE as we DON'T want Vulkan calls that cause a validation message 
	// (and return a VkResult) to abort
	// If you instead want to have calls abort, pass in VK_TRUE and the function will 
	// return VK_ERROR_VALIDATION_FAILED_EXT 
	return VK_FALSE;
}


DeviceManager::DeviceManager(const DeviceManagerDesc& desc)
	: m_desc{ desc }
{
	m_bIsDeveloperModeEnabled = IsDeveloperModeEnabled();
	m_bIsRenderDocAvailable = IsRenderDocAvailable();

	extern Luna::IDeviceManager* g_deviceManager;
	assert(!g_deviceManager);

	g_deviceManager = this;
	g_vulkanDeviceManager = this;
}


DeviceManager::~DeviceManager()
{
	extern Luna::IDeviceManager* g_deviceManager;
	g_deviceManager = nullptr;
	g_vulkanDeviceManager = nullptr;
}


void DeviceManager::BeginFrame()
{ 
	VkFence waitFence = *m_presentFences[m_activeFrame];
	vkWaitForFences(*m_vkDevice, 1, &waitFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetFences(*m_vkDevice, 1, &waitFence);

	VkSemaphore semaphore = *m_presentCompleteSemaphores[m_presentCompleteSemaphoreIndex];
	if (VK_FAILED(vkAcquireNextImageKHR(*m_vkDevice, *m_vkSwapChain, numeric_limits<uint64_t>::max(), semaphore, VK_NULL_HANDLE, &m_swapChainIndex)))
	{
		LogFatal(LogVulkan) << "Failed to acquire next swapchain image in BeginFrame.  Error code: " << res << endl;
		return;
	}

	QueueWaitForSemaphore(QueueType::Graphics, semaphore, 0);

	m_presentCompleteSemaphoreIndex = (m_presentCompleteSemaphoreIndex + 1) % m_presentCompleteSemaphores.size();
}


void DeviceManager::Present()
{ 
	VkSemaphore renderCompleteSemaphore = *m_renderCompleteSemaphores[m_renderCompleteSemaphoreIndex];

	// Kick the render complete semaphore
	Queue& graphicsQueue = GetQueue(QueueType::Graphics);
	graphicsQueue.AddSignalSemaphore(renderCompleteSemaphore, 0);
	graphicsQueue.AddWaitSemaphore(graphicsQueue.GetTimelineSemaphore(), graphicsQueue.GetLastSubmittedFenceValue());
	graphicsQueue.ExecuteCommandList(VK_NULL_HANDLE, *m_presentFences[m_activeFrame]);

	VkSwapchainKHR swapchain = *m_vkSwapChain;

	VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.pImageIndices = &m_swapChainIndex;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderCompleteSemaphore;

	vkQueuePresentKHR(graphicsQueue.GetVkQueue(), &presentInfo);

	m_activeFrame = (m_activeFrame + 1) % m_presentFences.size();
	m_renderCompleteSemaphoreIndex = (m_renderCompleteSemaphoreIndex + 1) % m_renderCompleteSemaphores.size();
}


void DeviceManager::WaitForGpu()
{ 
	for (auto& queue : m_queues)
	{
		queue->WaitForIdle();
	}
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
	// Initialize Volk
	if (VK_FAILED(volkInitialize()))
	{
		LogFatal(LogVulkan) << "Failed to initialize Volk.  Error code: " << res << endl;
		return;
	}

	if (!m_extensionManager.InitializeInstance())
	{
		LogFatal(LogVulkan) << "Failed to initialize instance extensions." << endl;
		return;
	}

	SetRequiredInstanceLayersAndExtensions();

	VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.pApplicationName = m_desc.appName.c_str();
	appInfo.pEngineName = s_engineVersionStr.c_str();
	appInfo.apiVersion = VK_API_VERSION_1_3;

	vector<const char*> instanceExtensions;
	vector<const char*> instanceLayers;
	m_extensionManager.GetEnabledInstanceExtensions(instanceExtensions);
	m_extensionManager.GetEnabledInstanceLayers(instanceLayers);

	VkInstanceCreateInfo createInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();
	createInfo.enabledLayerCount = (uint32_t)instanceLayers.size();
	createInfo.ppEnabledLayerNames = instanceLayers.data();

	VkInstance vkInstance{ VK_NULL_HANDLE };
	if (VK_FAILED(vkCreateInstance(&createInfo, nullptr, &vkInstance)))
	{
		LogFatal(LogVulkan) << "Failed to create Vulkan instance.  Error code: " << res << endl;
		return;
	}

	uint32_t instanceVersion{ 0 };
	if (VK_SUCCEEDED(vkEnumerateInstanceVersion(&instanceVersion)))
	{
		m_versionInfo = DecodeVulkanVersion(instanceVersion);

		LogInfo(LogVulkan) << format("Created Vulkan instance, variant {}, API version {}",
			m_versionInfo.variant, m_versionInfo) << endl;
	}
	else
	{
		LogInfo(LogVulkan) << "Created Vulkan instance, but failed to enumerate version.  Error code: " << res << endl;
	}

	volkLoadInstanceOnly(vkInstance);

	m_vkInstance = Create<CVkInstance>(vkInstance);

	if (m_desc.enableValidation)
	{
		InstallDebugMessenger();
	}

	CreateSurface();

	SelectPhysicalDevice();

	CreateDevice();

	// Create queues
	CreateQueue(QueueType::Graphics);
	CreateQueue(QueueType::Compute);
	CreateQueue(QueueType::Copy);

	// Create the semaphores and fences for present
	m_presentCompleteSemaphores.reserve(m_desc.maxFramesInFlight + 1);
	m_renderCompleteSemaphores.reserve(m_desc.maxFramesInFlight + 1);
	m_presentFences.reserve(m_desc.maxFramesInFlight + 1);
	for (uint32_t i = 0; i < m_desc.maxFramesInFlight + 1; ++i)
	{
		// Create present-complete semaphore
		{
			auto semaphore = m_device->CreateSemaphore(VK_SEMAPHORE_TYPE_BINARY, 0);
			assert(semaphore);
			m_presentCompleteSemaphores.push_back(semaphore);
			SetDebugName(*m_vkDevice, semaphore->Get(), format("Present Complete Semaphore {}", i));
		}

		// Create render-complete semaphore
		{
			auto semaphore = m_device->CreateSemaphore(VK_SEMAPHORE_TYPE_BINARY, 0);
			assert(semaphore);
			m_renderCompleteSemaphores.push_back(semaphore);
			SetDebugName(*m_vkDevice, semaphore->Get(), format("Render Complete Semaphore {}", i));
		}

		// Create fence
		auto fence = m_device->CreateFence(true);
		assert(fence);
		m_presentFences.push_back(fence);
		SetDebugName(*m_vkDevice, fence->Get(), format("Present Fence {}", i));
	}
}


void DeviceManager::CreateWindowSizeDependentResources()
{ 
	m_swapChainFormat = { FormatToVulkan(RemoveSrgb(m_desc.swapChainFormat)), VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

	VkExtent2D extent{
		m_desc.backBufferWidth,
		m_desc.backBufferHeight };

	unordered_set<uint32_t> uniqueQueues{
		(uint32_t)m_queueFamilyIndices.graphics,
		(uint32_t)m_queueFamilyIndices.present };
	vector<uint32_t> queues(uniqueQueues.begin(), uniqueQueues.end());

	VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	m_swapChainMutableFormatSupported = true;

	const bool enableSwapChainSharing = queues.size() > 1;

	VkSwapchainCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	createInfo.surface = *m_vkSurface;
	createInfo.minImageCount = m_desc.numSwapChainBuffers;
	createInfo.imageFormat = m_swapChainFormat.format;
	createInfo.imageColorSpace = m_swapChainFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = usage;
	createInfo.imageSharingMode = enableSwapChainSharing ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
	createInfo.flags = m_swapChainMutableFormatSupported ? VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR : 0;
	createInfo.queueFamilyIndexCount = enableSwapChainSharing ? (uint32_t)queues.size() : 0;
	createInfo.pQueueFamilyIndices = enableSwapChainSharing ? queues.data() : nullptr;
	createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = m_desc.enableVSync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
	createInfo.clipped = true;
	createInfo.oldSwapchain = nullptr;

	vector<VkFormat> imageFormats{ m_swapChainFormat.format };
	switch (m_swapChainFormat.format)
	{
	case VK_FORMAT_R8G8B8A8_UNORM:
		imageFormats.push_back(VK_FORMAT_R8G8B8A8_SRGB);
		break;
	case VK_FORMAT_R8G8B8A8_SRGB:
		imageFormats.push_back(VK_FORMAT_R8G8B8A8_UNORM);
		break;
	case VK_FORMAT_B8G8R8A8_UNORM:
		imageFormats.push_back(VK_FORMAT_B8G8R8A8_SRGB);
		break;
	case VK_FORMAT_B8G8R8A8_SRGB:
		imageFormats.push_back(VK_FORMAT_B8G8R8A8_UNORM);
		break;
	}

	VkImageFormatListCreateInfo imageFormatListCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO };
	imageFormatListCreateInfo.viewFormatCount = (uint32_t)imageFormats.size();
	imageFormatListCreateInfo.pViewFormats = imageFormats.data();

	if (m_swapChainMutableFormatSupported)
	{
		createInfo.pNext = &imageFormatListCreateInfo;
	}

	VkSwapchainKHR swapchain{ VK_NULL_HANDLE };
	if (VK_FAILED(vkCreateSwapchainKHR(*m_vkDevice, &createInfo, nullptr, &swapchain)))
	{
		LogError(LogVulkan) << "Failed to create Vulkan swapchain.  Error code: " << res << endl;
		return;
	}
	m_vkSwapChain = Create<CVkSwapchain>(m_vkDevice.get(), swapchain);

	// Get swapchain images
	uint32_t imageCount{ 0 };
	if (VK_FAILED(vkGetSwapchainImagesKHR(*m_vkDevice, *m_vkSwapChain, &imageCount, nullptr)))
	{
		LogError(LogVulkan) << "Failed to get swapchain image count.  Error code: " << res << endl;
		return;
	}

	vector<VkImage> images{ imageCount };
	if (VK_FAILED(vkGetSwapchainImagesKHR(*m_vkDevice, *m_vkSwapChain, &imageCount, images.data())))
	{
		LogError(LogVulkan) << "Failed to get swapchain images.  Error code: " << res << endl;
		return;
	}
	m_vkSwapChainImages.reserve(imageCount);
	for (auto image : images)
	{
		m_vkSwapChainImages.push_back(Create<CVkImage>(m_vkDevice.get(), image));
	}

	m_swapChainBuffers.reserve(imageCount);
	for (uint32_t i = 0; i < imageCount; ++i)
	{
		ColorBuffer buffer;
		buffer.InitializeFromSwapchain(i);
		m_swapChainBuffers.push_back(buffer);
	}
}


ICommandContext* DeviceManager::AllocateContext(CommandListType commandListType)
{
	lock_guard<mutex> lockGuard(m_contextAllocationMutex);

	auto& availableContexts = m_availableContexts[(uint32_t)commandListType];

	ICommandContext* retPtr{ nullptr };
	wil::com_ptr<ICommandContext> ret;
	if (availableContexts.empty())
	{
		switch (commandListType)
		{
		case CommandListType::Direct:
		{
			wil::com_ptr<GraphicsContext> graphicsContext = Make<GraphicsContext>();
			ret = graphicsContext.query<ICommandContext>();
		}
		break;
		case CommandListType::Compute:
		{
			wil::com_ptr<ComputeContext> computeContext = Make<ComputeContext>();
			ret = computeContext.query<ICommandContext>();
		}
		break;
		} // switch

		retPtr = ret.get();
		m_contextPool[(uint32_t)commandListType].emplace_back(ret);
		retPtr->Initialize();
	}
	else
	{
		retPtr = availableContexts.front();
		availableContexts.pop();
		retPtr->Reset();
	}

	assert(retPtr != nullptr);
	assert(retPtr->GetType() == commandListType);

	return retPtr;
}


void DeviceManager::FreeContext(ICommandContext* usedContext)
{
	lock_guard<mutex> guard{ m_contextAllocationMutex };

	m_availableContexts[(uint32_t)usedContext->GetType()].push(usedContext);
}


wil::com_ptr<IPlatformData> DeviceManager::CreateColorBufferFromSwapChain(ColorBufferDesc& desc, ResourceState& initialState, uint32_t imageIndex)
{
	const string name = format("Primary Swapchain Image {}", imageIndex);

	// Swapchain image
	desc = ColorBufferDesc{
		.name = name,
		.resourceType = ResourceType::Texture2D,
		.width = m_desc.backBufferWidth,
		.height = m_desc.backBufferHeight,
		.arraySizeOrDepth = 1,
		.numSamples = 1,
		.format = m_desc.swapChainFormat
	};

	auto image = Create<CVkImage>(m_vkDevice.get(), m_vkSwapChainImages[imageIndex]->Get());
	SetDebugName(*m_vkDevice, image->Get(), name);

	// RTV view
	auto imageViewDesc = ImageViewDesc{
		.image = image.get(),
		.name = format("Primary Swapchain {} RTV Image View", imageIndex),
		.resourceType = ResourceType::Texture2D,
		.imageUsage = GpuImageUsage::RenderTarget,
		.format = m_desc.swapChainFormat,
		.imageAspect = ImageAspect::Color,
		.baseMipLevel = 0,
		.mipCount = 1,
		.baseArraySlice = 0,
		.arraySize = 1
	};

	auto imageViewRtv = m_device->CreateImageView(imageViewDesc);

	// SRV view
	imageViewDesc
		.SetImageUsage(GpuImageUsage::ShaderResource)
		.SetName(format("Primary SwapChain {} SRV Image View", imageIndex));

	auto imageViewSrv = m_device->CreateImageView(imageViewDesc);

	// Descriptors
	VkDescriptorImageInfo imageInfoSrv{ VK_NULL_HANDLE, *imageViewSrv, GetImageLayout(ResourceState::ShaderResource) };
	VkDescriptorImageInfo imageInfoUav{ VK_NULL_HANDLE, *imageViewSrv, GetImageLayout(ResourceState::UnorderedAccess) };

	initialState = ResourceState::Undefined;

	auto descExt = ColorBufferDescExt{
		.image = image.get(),
		.imageViewRtv = imageViewRtv.get(),
		.imageViewSrv = imageViewSrv.get(),
		.imageInfoSrv = imageInfoSrv,
		.imageInfoUav = imageInfoUav,
		.usageState = initialState
	};

	return Make<ColorBufferData>(descExt);
}


ColorBuffer& DeviceManager::GetColorBuffer()
{
	return m_swapChainBuffers[m_swapChainIndex];
}


void DeviceManager::SetRequiredInstanceLayersAndExtensions()
{
	vector<string> requiredLayers{};
	if (m_desc.enableValidation)
	{
		requiredLayers.push_back("VK_LAYER_KHRONOS_validation");
	}
	m_extensionManager.SetRequiredInstanceLayers(requiredLayers);

	vector<string> requiredExtensions{
		"VK_EXT_debug_utils",
		"VK_KHR_win32_surface",
		"VK_KHR_surface"
	};
	m_extensionManager.SetRequiredInstanceExtensions(requiredExtensions);
}


void DeviceManager::InstallDebugMessenger()
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	createInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	createInfo.pfnUserCallback = DebugMessageCallback;

	VkDebugUtilsMessengerEXT messenger{ VK_NULL_HANDLE };
	if (VK_FAILED(vkCreateDebugUtilsMessengerEXT(*m_vkInstance, &createInfo, nullptr, &messenger)))
	{
		LogWarning(LogVulkan) << "Failed to create Vulkan debug messenger.  Error Code: " << res << endl;
		return;
	}

	m_vkDebugMessenger = Create<CVkDebugUtilsMessenger>(m_vkInstance.get(), messenger);
}


void DeviceManager::CreateSurface()
{
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	surfaceCreateInfo.hinstance = m_desc.hinstance;
	surfaceCreateInfo.hwnd = m_desc.hwnd;

	VkSurfaceKHR vkSurface{ VK_NULL_HANDLE };
	if (VK_FAILED(vkCreateWin32SurfaceKHR(*m_vkInstance, &surfaceCreateInfo, nullptr, &vkSurface)))
	{
		LogError(LogVulkan) << "Failed to create Win32 surface.  Error code: " << res << endl;
		return;
	}
	m_vkSurface = Create<CVkSurface>(m_vkInstance.get(), vkSurface);
}


void DeviceManager::SelectPhysicalDevice()
{ 
	using enum AdapterType;

	vector<pair<AdapterInfo, VkPhysicalDevice>> physicalDevices = EnumeratePhysicalDevices();

	if (physicalDevices.empty())
	{
		LogFatal(LogVulkan) << "Failed to enumerate any physical devices." << endl;
		return;
	}

	int32_t firstDiscreteAdapterIdx{ -1 };
	int32_t bestMemoryAdapterIdx{ -1 };
	int32_t firstAdapterIdx{ -1 };
	int32_t softwareAdapterIdx{ -1 };
	int32_t chosenAdapterIdx{ -1 };
	size_t maxMemory{ 0 };

	int32_t adapterIdx{ 0 };
	for (const auto& adapterPair : physicalDevices)
	{
		// Skip adapters that don't support the required Vulkan API version
		if (adapterPair.first.apiVersion < g_requiredVulkanApiVersion)
		{
			continue;
		}

		// Skip adapters of type 'Other'
		if (adapterPair.first.adapterType == Other)
		{
			continue;
		}

		// Skip software adapters if we disallow them
		if (adapterPair.first.adapterType == Software && !m_desc.allowSoftwareDevice)
		{
			continue;
		}

		if (firstAdapterIdx == -1)
		{
			firstAdapterIdx = adapterIdx;
		}

		if (adapterPair.first.adapterType == Discrete && firstDiscreteAdapterIdx == -1)
		{
			firstDiscreteAdapterIdx = adapterIdx;
		}

		if (adapterPair.first.adapterType == Software && softwareAdapterIdx == -1 && m_desc.allowSoftwareDevice)
		{
			softwareAdapterIdx = adapterIdx;
		}

		if (adapterPair.first.dedicatedVideoMemory > maxMemory)
		{
			maxMemory = adapterPair.first.dedicatedVideoMemory;
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
		LogFatal(LogVulkan) << "Failed to select a Vulkan physical device." << endl;
		return;
	}

	m_vkPhysicalDevice = Create<CVkPhysicalDevice>(m_vkInstance.get(), physicalDevices[chosenAdapterIdx].second);
	LogInfo(LogVulkan) << "Selected physical device " << chosenAdapterIdx << endl;

	// TODO
	m_caps.ReadCaps(*m_vkPhysicalDevice);
	//if (g_graphicsDeviceOptions.logDeviceFeatures)
	if (true)
	{
		m_caps.LogCaps();
	}

	m_extensionManager.InitializeDevice(*m_vkPhysicalDevice);

	// Get available queue family properties
	uint32_t queueCount{ 0 };
	vkGetPhysicalDeviceQueueFamilyProperties(*m_vkPhysicalDevice, &queueCount, nullptr);
	assert(queueCount >= 1);

	m_queueFamilyProperties.resize(queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(*m_vkPhysicalDevice, &queueCount, m_queueFamilyProperties.data());

	GetQueueFamilyIndices();
}


void DeviceManager::CreateDevice()
{ 
	// Desired queues need to be requested upon logical device creation
	// Due to differing queue family configurations of Vulkan implementations this can be a bit tricky, especially if the application
	// requests different queue types

	vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

	// Get queue family indices for the requested queue family types
	// Note that the indices may overlap depending on the implementation

	const float defaultQueuePriority = 0.0f;

	// Graphics queue
	if (m_queueFamilyIndices.graphics != -1)
	{
		VkDeviceQueueCreateInfo queueInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		queueInfo.queueFamilyIndex = m_queueFamilyIndices.graphics;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &defaultQueuePriority;
		queueCreateInfos.push_back(queueInfo);
	}
	else
	{
		LogError(LogVulkan) << "Failed to find graphics queue." << endl;
	}

	// Dedicated compute queue
	if (m_queueFamilyIndices.compute != -1)
	{
		if (m_queueFamilyIndices.compute != m_queueFamilyIndices.graphics)
		{
			// If compute family index differs, we need an additional queue create info for the compute queue
			VkDeviceQueueCreateInfo queueInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
			queueInfo.queueFamilyIndex = m_queueFamilyIndices.compute;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &defaultQueuePriority;
			queueCreateInfos.push_back(queueInfo);
		}
	}
	else
	{
		LogError(LogVulkan) << "Failed to find compute queue." << endl;
	}

	// Dedicated transfer queue
	if (m_queueFamilyIndices.transfer != -1)
	{
		if ((m_queueFamilyIndices.transfer != m_queueFamilyIndices.graphics) && (m_queueFamilyIndices.transfer != m_queueFamilyIndices.compute))
		{
			// If compute family index differs, we need an additional queue create info for the transfer queue
			VkDeviceQueueCreateInfo queueInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
			queueInfo.queueFamilyIndex = m_queueFamilyIndices.transfer;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &defaultQueuePriority;
			queueCreateInfos.push_back(queueInfo);
		}
	}
	else
	{
		LogError(LogVulkan) << "Failed to find transfer queue." << endl;
	}

	// HACK - replace with real device extension handling
	vector<const char*> extensions{ "VK_KHR_swapchain", "VK_KHR_swapchain_mutable_format" };

	VkDeviceCreateInfo createInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = nullptr;
	// TODO - device extensions
	createInfo.enabledExtensionCount = (uint32_t)extensions.size();;
	createInfo.ppEnabledExtensionNames = extensions.data();
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;
	createInfo.pNext = m_caps.GetPhysicalDeviceFeatures2();

	VkDevice device{ VK_NULL_HANDLE };
	if (VK_FAILED(vkCreateDevice(*m_vkPhysicalDevice, &createInfo, nullptr, &device)))
	{
		LogError(LogVulkan) << "Failed to create Vulkan device.  Error code: " << res << endl;
	}

	volkLoadDevice(device);

	m_vkDevice = Create<CVkDevice>(m_vkPhysicalDevice.get(), device);

	// Create Luna GraphicsDevice
	auto deviceDesc = GraphicsDeviceDesc{
		.instance				= *m_vkInstance,
		.physicalDevice			= m_vkPhysicalDevice.get(),
		.device					= m_vkDevice.get(),
		.queueFamilyIndices		= { 
			.graphics	= m_queueFamilyIndices.graphics,
			.compute	= m_queueFamilyIndices.compute,
			.transfer	= m_queueFamilyIndices.transfer,
			.present	= m_queueFamilyIndices.present },
		.backBufferWidth		= m_desc.backBufferWidth,
		.backBufferHeight		= m_desc.backBufferHeight,
		.numSwapChainBuffers	= m_desc.numSwapChainBuffers,
		.swapChainFormat		= m_desc.swapChainFormat,
		.surface				= *m_vkSurface,
		.enableVSync			= m_desc.enableVSync,
		.maxFramesInFlight		= m_desc.maxFramesInFlight,
		.enableValidation		= m_desc.enableValidation,
		.enableDebugMarkers		= m_desc.enableDebugMarkers
	};

	m_device = Make<GraphicsDevice>(deviceDesc);

	m_device->CreateResources();
}


void DeviceManager::CreateQueue(QueueType queueType)
{
	VkQueue vkQueue{ VK_NULL_HANDLE };
	vkGetDeviceQueue(*m_vkDevice, m_queueFamilyIndices.graphics, 0, &vkQueue);
	m_queues[(uint32_t)queueType] = make_unique<Queue>(m_device.get(), vkQueue, queueType);
}


void DeviceManager::ResizeSwapChain()
{
	WaitForGpu();

	m_vkSwapChain.reset();
	m_vkSwapChainImages.clear();
	m_swapChainBuffers.clear();

	CreateWindowSizeDependentResources();
}


vector<pair<AdapterInfo, VkPhysicalDevice>> DeviceManager::EnumeratePhysicalDevices()
{
	vector<pair<AdapterInfo, VkPhysicalDevice>> adapters;

	uint32_t gpuCount{ 0 };

	// Get number of available physical devices
	if (VK_FAILED(vkEnumeratePhysicalDevices(*m_vkInstance, &gpuCount, nullptr)))
	{
		LogError(LogVulkan) << "Failed to get physical device count.  Error code: " << res << endl;
		return adapters;
	}

	// Enumerate physical devices
	vector<VkPhysicalDevice> physicalDevices(gpuCount);
	if (VK_FAILED(vkEnumeratePhysicalDevices(*m_vkInstance, &gpuCount, physicalDevices.data())))
	{
		LogError(LogVulkan) << "Failed to enumerate physical devices.  Error code: " << res << endl;
		return adapters;
	}

	LogInfo(LogVulkan) << "Enumerating Vulkan physical devices" << endl;

	for (size_t deviceIdx = 0; deviceIdx < physicalDevices.size(); ++deviceIdx)
	{
		DeviceCaps caps{};
		caps.ReadCaps(physicalDevices[deviceIdx]);

		AdapterInfo adapterInfo{};
		adapterInfo.name = caps.properties.deviceName;
		adapterInfo.deviceId = caps.properties.deviceID;
		adapterInfo.vendorId = caps.properties.vendorID;
		adapterInfo.dedicatedVideoMemory = GetDedicatedVideoMemory(physicalDevices[deviceIdx]);
		adapterInfo.vendor = VendorIdToHardwareVendor(adapterInfo.vendorId);
		adapterInfo.adapterType = VkPhysicalDeviceTypeToEngine(caps.properties.deviceType);
		adapterInfo.apiVersion = caps.properties.apiVersion;

		LogInfo(LogVulkan) << format("  {} physical device {} is Vulkan-capable: {} (VendorId: {:#x}, DeviceId: {:#x}, API version: {})",
			AdapterTypeToString(adapterInfo.adapterType),
			deviceIdx,
			adapterInfo.name,
			adapterInfo.vendorId, adapterInfo.deviceId, VulkanVersionToString(adapterInfo.apiVersion))
			<< endl;

		LogInfo(LogVulkan) << format("    Physical device memory: {} MB dedicated video memory, {} MB dedicated system memory, {} MB shared memory",
			(uint32_t)(adapterInfo.dedicatedVideoMemory >> 20),
			(uint32_t)(adapterInfo.dedicatedSystemMemory >> 20),
			(uint32_t)(adapterInfo.sharedSystemMemory >> 20))
			<< endl;

		adapters.push_back(make_pair(adapterInfo, physicalDevices[deviceIdx]));
	}

	return adapters;
}


void DeviceManager::GetQueueFamilyIndices()
{
	m_queueFamilyIndices.graphics = GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
	m_queueFamilyIndices.compute = GetQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT);
	m_queueFamilyIndices.transfer = GetQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT);
}


int32_t DeviceManager::GetQueueFamilyIndex(VkQueueFlags queueFlags)
{
	int32_t index{ 0 };

	// Dedicated queue for compute
	// Try to find a queue family index that supports compute but not graphics
	if (queueFlags & VK_QUEUE_COMPUTE_BIT)
	{
		for (const auto& properties : m_queueFamilyProperties)
		{
			if ((properties.queueFlags & queueFlags) && ((properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
			{
				if (m_queueFamilyIndices.present == -1 && vkGetPhysicalDeviceWin32PresentationSupportKHR(*m_vkPhysicalDevice, index))
				{
					m_queueFamilyIndices.present = index;
				}

				return index;
			}
			++index;
		}
	}

	// Dedicated queue for transfer
	// Try to find a queue family index that supports transfer but not graphics and compute
	if (queueFlags & VK_QUEUE_TRANSFER_BIT)
	{
		index = 0;
		for (const auto& properties : m_queueFamilyProperties)
		{
			if ((properties.queueFlags & queueFlags) && ((properties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((properties.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
			{
				if (m_queueFamilyIndices.present == -1 && vkGetPhysicalDeviceWin32PresentationSupportKHR(*m_vkPhysicalDevice, index))
				{
					m_queueFamilyIndices.present = index;
				}

				return index;
			}
			++index;
		}
	}

	// For other queue types or if no separate compute queue is present, return the first one to support the requested flags
	index = 0;
	for (const auto& properties : m_queueFamilyProperties)
	{
		if (properties.queueFlags & queueFlags)
		{
			if (m_queueFamilyIndices.present == -1 && vkGetPhysicalDeviceWin32PresentationSupportKHR(*m_vkPhysicalDevice, index))
			{
				m_queueFamilyIndices.present = index;
			}

			return index;
		}
		++index;
	}

	LogWarning(LogVulkan) << "Failed to find a matching queue family index for queue bit(s) " << VkQueueFlagsToString(queueFlags) << endl;
	return -1;
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


void DeviceManager::QueueWaitForSemaphore(QueueType queueType, VkSemaphore semaphore, uint64_t value)
{
	m_queues[(uint32_t)queueType]->AddWaitSemaphore(semaphore, value);
}


void DeviceManager::QueueSignalSemaphore(QueueType queueType, VkSemaphore semaphore, uint64_t value)
{
	m_queues[(uint32_t)queueType]->AddSignalSemaphore(semaphore, value);
}


DeviceManager* GetVulkanDeviceManager()
{
	return g_vulkanDeviceManager;
}

} // namespace Luna::VK