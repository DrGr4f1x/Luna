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

#include "ColorBufferPoolVK.h"
#include "CommandContextVK.h"
#include "DepthBufferPoolVK.h"
#include "DescriptorAllocatorVK.h"
#include "DescriptorSetPoolVK.h"
#include "GpuBufferPoolVK.h"
#include "PipelineStatePoolVK.h"
#include "QueueVK.h"
#include "RootSignaturePoolVK.h"
#include "VulkanUtil.h"

using namespace std;
using namespace Microsoft::WRL;

#ifdef CreateSemaphore
#undef CreateSemaphore
#endif


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
		__debugbreak();
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
	WaitForGpu();

	// Flush pending deferred resources here
	ReleaseDeferredResources();
	assert(m_deferredResources.empty());

	DescriptorSetAllocator::DestroyAll();

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

	ReleaseDeferredResources();
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

	if (!m_extensionManager.InitializeSystem())
	{
		LogFatal(LogVulkan) << "Failed to get Vulkan system info." << endl;
		return;
	}

	// Create Vulkan instance
	vkb::InstanceBuilder instanceBuilder;
	instanceBuilder.set_app_name(m_desc.appName.c_str());
	instanceBuilder.set_engine_name(s_engineVersionStr.c_str());
	instanceBuilder.require_api_version(VK_API_VERSION_1_3);
	instanceBuilder.request_validation_layers(m_desc.enableValidation);
	if (m_desc.enableValidation)
	{
		InstallDebugMessenger(instanceBuilder);
	}

	SetRequiredInstanceLayersAndExtensions(instanceBuilder);
	m_extensionManager.EnableInstanceExtensions(instanceBuilder);
	m_extensionManager.EnableInstanceLayers(instanceBuilder);

	auto instanceRet = instanceBuilder.build();
	if (!instanceRet)
	{
		LogFatal(LogVulkan) << "Failed to create Vulkan instance." << endl;
		return;
	}
	m_vkbInstance = instanceRet.value();

	m_vkInstance = Create<CVkInstance>(m_vkbInstance.instance);
	m_vkDebugMessenger = Create<CVkDebugUtilsMessenger>(m_vkInstance.get(), m_vkbInstance.debug_messenger);

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

	// Load instance functions
	volkLoadInstanceOnly(m_vkInstance->Get());

	CreateSurface();

	vkb::PhysicalDeviceSelector physicalDeviceSelector(m_vkbInstance);
	physicalDeviceSelector.set_surface(m_vkSurface->Get());
	auto physicalDeviceRet = physicalDeviceSelector.select();
	if (!physicalDeviceRet)
	{
		LogFatal(LogVulkan) << "Failed to select Vulkan physical device." << endl;
		return;
	}

	m_vkbPhysicalDevice = physicalDeviceRet.value();
	m_vkPhysicalDevice = Create<CVkPhysicalDevice>(m_vkInstance.get(), m_vkbPhysicalDevice.physical_device);	

	m_queueFamilyProperties = m_vkbPhysicalDevice.get_queue_families();
	for (const auto& queueFamily : m_queueFamilyProperties)
	{
		LogInfo(LogVulkan) << "Queue family: " << queueFamily.queueFlags << " " << VkQueueFlagsToString(queueFamily.queueFlags) << endl;
	}

	GetQueueFamilyIndices();

	// TODO
	m_caps.ReadCaps(*m_vkPhysicalDevice);
	//if (g_graphicsDeviceOptions.logDeviceFeatures)
	if (true)
	{
		m_caps.LogCaps();
	}

	// Device extensions
	m_extensionManager.InitializeDevice(m_vkbPhysicalDevice);
	SetRequiredDeviceExtensions(m_vkbPhysicalDevice);

	CreateDevice();

	CreateResourcePools();

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
			auto semaphore = CreateSemaphore(m_vkDevice.get(), VK_SEMAPHORE_TYPE_BINARY, 0);
			assert(semaphore);
			m_presentCompleteSemaphores.push_back(semaphore);
			SetDebugName(*m_vkDevice, semaphore->Get(), format("Present Complete Semaphore {}", i));
		}

		// Create render-complete semaphore
		{
			auto semaphore = CreateSemaphore(m_vkDevice.get(), VK_SEMAPHORE_TYPE_BINARY, 0);
			assert(semaphore);
			m_renderCompleteSemaphores.push_back(semaphore);
			SetDebugName(*m_vkDevice, semaphore->Get(), format("Render Complete Semaphore {}", i));
		}

		// Create fence
		auto fence = CreateFence(m_vkDevice.get(), true);
		assert(fence);
		m_presentFences.push_back(fence);
		SetDebugName(*m_vkDevice, fence->Get(), format("Present Fence {}", i));
	}
}


void DeviceManager::CreateWindowSizeDependentResources()
{ 
	m_swapChainFormat = RemoveSrgb(m_desc.swapChainFormat);
	m_swapChainSurfaceFormat = { FormatToVulkan(m_swapChainFormat), VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

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
	createInfo.imageFormat = m_swapChainSurfaceFormat.format;
	createInfo.imageColorSpace = m_swapChainSurfaceFormat.colorSpace;
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

	vector<VkFormat> imageFormats{ m_swapChainSurfaceFormat.format };
	switch (m_swapChainSurfaceFormat.format)
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
		ColorBuffer swapChainBuffer;
		swapChainBuffer.SetHandle(m_colorBufferPool->CreateColorBufferFromSwapChainImage(m_vkSwapChainImages[i].get(), m_desc.backBufferWidth, m_desc.backBufferHeight, m_swapChainFormat, i).get());
		m_swapChainBuffers.push_back(swapChainBuffer);
	}
}


CommandContext* DeviceManager::AllocateContext(CommandListType commandListType)
{
	lock_guard<mutex> lockGuard(m_contextAllocationMutex);

	auto& availableContexts = m_availableContexts[(uint32_t)commandListType];

	CommandContext* ret{ nullptr };
	if (availableContexts.empty())
	{
		wil::com_ptr<ICommandContext> contextImpl = Make<CommandContextVK>(commandListType);
		ret = new CommandContext(contextImpl.get());

		m_contextPool[(uint32_t)commandListType].emplace_back(ret);
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


void DeviceManager::FreeContext(CommandContext* usedContext)
{
	lock_guard<mutex> guard{ m_contextAllocationMutex };

	m_availableContexts[(uint32_t)usedContext->GetType()].push(usedContext);
}


ColorBuffer& DeviceManager::GetColorBuffer()
{
	return m_swapChainBuffers[m_swapChainIndex];
}


Format DeviceManager::GetColorFormat()
{
	return m_swapChainFormat;
}


Format DeviceManager::GetDepthFormat()
{
	return m_desc.depthBufferFormat;
}


IColorBufferPool* DeviceManager::GetColorBufferPool()
{
	return m_colorBufferPool.get();
}


IDepthBufferPool* DeviceManager::GetDepthBufferPool()
{
	return m_depthBufferPool.get();
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


void DeviceManager::ReleaseImage(CVkImage* image)
{
	uint64_t nextFence = GetQueue(QueueType::Graphics).GetNextFenceValue();

	DeferredReleaseResource resource{ nextFence, image, nullptr };
	m_deferredResources.emplace_back(resource);
}


void DeviceManager::ReleaseBuffer(CVkBuffer* buffer)
{
	uint64_t nextFence = GetQueue(QueueType::Graphics).GetNextFenceValue();

	DeferredReleaseResource resource{ nextFence, nullptr, buffer };
	m_deferredResources.emplace_back(resource);
}


CVkDevice* DeviceManager::GetDevice() const
{
	return m_vkDevice.get();
}


CVmaAllocator* DeviceManager::GetAllocator() const
{
	return m_vmaAllocator.get();
}


void DeviceManager::SetRequiredInstanceLayersAndExtensions(vkb::InstanceBuilder& instanceBuilder)
{
	vector<string> requiredLayers{};
	if (m_desc.enableValidation)
	{
		requiredLayers.push_back("VK_LAYER_KHRONOS_validation");
	}
	m_extensionManager.SetRequiredInstanceLayers(requiredLayers);
	if (!m_extensionManager.EnableInstanceLayers(instanceBuilder))
	{
		LogFatal(LogVulkan) << "Failed to enable required instance layers." << endl;
	}

	vector<string> requiredExtensions{
		"VK_EXT_debug_utils",
		"VK_KHR_win32_surface",
		"VK_KHR_surface"
	};
	m_extensionManager.SetRequiredInstanceExtensions(requiredExtensions);
	if (!m_extensionManager.EnableInstanceExtensions(instanceBuilder))
	{
		LogFatal(LogVulkan) << "Failed to enable required instance extensions." << endl;
	}
}


void DeviceManager::SetRequiredDeviceExtensions(vkb::PhysicalDevice& physicalDevice)
{
	vector<string> requiredExtensions{ 
		"VK_KHR_swapchain",
		"VK_KHR_swapchain_mutable_format",
		"VK_EXT_descriptor_buffer"
	};
	m_extensionManager.SetRequiredDeviceExtensions(requiredExtensions);
	if (!m_extensionManager.EnableDeviceExtensions(physicalDevice))
	{
		LogFatal(LogVulkan) << "Failed to enable required device extensions." << endl;
	}
}


void DeviceManager::InstallDebugMessenger(vkb::InstanceBuilder& instanceBuilder)
{
	instanceBuilder.set_debug_callback(DebugMessageCallback);
	instanceBuilder.set_debug_messenger_severity(
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT);
	instanceBuilder.set_debug_messenger_type(
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT);
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


void DeviceManager::CreateDevice()
{
	auto deviceBuilder = vkb::DeviceBuilder(m_vkbPhysicalDevice);

	vector<vkb::CustomQueueDescription> queueDescriptions;

	// Graphics queue
	if (m_queueFamilyIndices.graphics != -1)
	{
		queueDescriptions.push_back(vkb::CustomQueueDescription(m_queueFamilyIndices.graphics, { 0.0f }));
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
			queueDescriptions.push_back(vkb::CustomQueueDescription(m_queueFamilyIndices.compute, { 0.0f }));
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
			queueDescriptions.push_back(vkb::CustomQueueDescription(m_queueFamilyIndices.transfer, { 0.0f }));
		}
	}
	else
	{
		LogError(LogVulkan) << "Failed to find transfer queue." << endl;
	}

	deviceBuilder.add_pNext(m_caps.GetPhysicalDeviceFeatures2());

	auto deviceRet = deviceBuilder.build();
	if (!deviceRet)
	{
		LogFatal(LogVulkan) << "Failed to create Vulkan device." << endl;
	}

	m_vkbDevice = deviceRet.value();

	m_vkDevice = Create<CVkDevice>(m_vkPhysicalDevice.get(), m_vkbDevice.device);

	volkLoadDevice(m_vkDevice->Get());

	// Create VmaAllocator
	VmaVulkanFunctions vmaFunctions{};
	vmaFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vmaFunctions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

	VmaAllocatorCreateInfo allocatorCreateInfo{};
	allocatorCreateInfo.physicalDevice = *m_vkPhysicalDevice;
	allocatorCreateInfo.device = *m_vkDevice;
	allocatorCreateInfo.instance = *m_vkInstance;
	allocatorCreateInfo.pVulkanFunctions = &vmaFunctions;

	VmaAllocator vmaAllocator{ VK_NULL_HANDLE };
	if (VK_SUCCEEDED(vmaCreateAllocator(&allocatorCreateInfo, &vmaAllocator)))
	{
		m_vmaAllocator = Create<CVmaAllocator>(m_vkDevice.get(), vmaAllocator);
	}
	else
	{
		LogError(LogVulkan) << "Failed to create VmaAllocator.  Error code: " << res << endl;
	}
}


void DeviceManager::CreateResourcePools()
{
	m_colorBufferPool = make_unique<ColorBufferPool>(m_vkDevice.get(), m_vmaAllocator.get());
	m_depthBufferPool = make_unique<DepthBufferPool>(m_vkDevice.get(), m_vmaAllocator.get());
	m_descriptorSetPool = make_unique<DescriptorSetPool>(m_vkDevice.get());
	m_gpuBufferPool = make_unique<GpuBufferPool>(m_vkDevice.get(), m_vmaAllocator.get());
	m_pipelineStatePool = make_unique<PipelineStatePool>(m_vkDevice.get());
	m_rootSignaturePool = make_unique<RootSignaturePool>(m_vkDevice.get());
}


void DeviceManager::CreateQueue(QueueType queueType)
{
	VkQueue vkQueue{ VK_NULL_HANDLE };
	
	uint32_t queueFamilyIndex{ 0 };
	switch (queueType)
	{
	case QueueType::Graphics:	queueFamilyIndex = m_queueFamilyIndices.graphics; break;
	case QueueType::Compute:	queueFamilyIndex = m_queueFamilyIndices.compute; break;
	case QueueType::Copy:		queueFamilyIndex = m_queueFamilyIndices.transfer; break;
	default:					queueFamilyIndex = m_queueFamilyIndices.present; break;
	}

	vkGetDeviceQueue(*m_vkDevice, queueFamilyIndex, 0, &vkQueue);
	m_queues[(uint32_t)queueType] = make_unique<Queue>(m_vkDevice.get(), vkQueue, queueType, queueFamilyIndex);
}


void DeviceManager::ResizeSwapChain()
{
	WaitForGpu();

	m_vkSwapChain.reset();
	m_vkSwapChainImages.clear();
	m_swapChainBuffers.clear();

	CreateWindowSizeDependentResources();
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


DeviceManager* GetVulkanDeviceManager()
{
	return g_vulkanDeviceManager;
}

} // namespace Luna::VK