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

#include "CommandContextVK.h"
#include "DescriptorAllocatorVK.h"
#include "DeviceVK.h"
#include "LinearAllocatorVK.h"
#include "SemaphoreVK.h"
#include "QueueVK.h"
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

	prefix += format("[Frame {}] ", GetFrameNumber());

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


Limits::Limits(VkPhysicalDeviceLimits limits)
	: m_limits{ limits }
{
	extern Luna::ILimits* g_limits;
	assert(g_limits == nullptr);

	g_limits = this;
}


Limits::~Limits()
{
	extern Luna::ILimits* g_limits;
	g_limits = nullptr;
}


DeviceManager::DeviceManager(const DeviceManagerDesc& desc)
	: m_desc{ desc }
{
	m_bIsDeveloperModeEnabled = IsDeveloperModeEnabled();
	m_bIsRenderDocAvailable = IsRenderDocAvailable();

	// Can't use the validation layer if the application is launched through RenderDoc
	m_desc.enableValidation = m_bIsRenderDocAvailable ? false : desc.enableValidation;

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
	LinearAllocator::DestroyAll();

	extern Luna::IDeviceManager* g_deviceManager;
	g_deviceManager = nullptr;
	g_vulkanDeviceManager = nullptr;
}


void DeviceManager::BeginFrame()
{ 
	ScopedEvent event{ "DeviceManager::BeginFrame" };

	VkFence waitFence = *m_presentFences[m_activeFrame];
	vkWaitForFences(*m_vkDevice, 1, &waitFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetFences(*m_vkDevice, 1, &waitFence);

	auto presentCompleteSemaphore = m_presentCompleteSemaphores[m_presentCompleteSemaphoreIndex];

	VkSemaphore semaphore = presentCompleteSemaphore->semaphore->Get();
	if (VK_FAILED(vkAcquireNextImageKHR(*m_vkDevice, *m_vkSwapChain, numeric_limits<uint64_t>::max(), semaphore, VK_NULL_HANDLE, &m_swapChainIndex)))
	{
		LogFatal(LogVulkan) << "Failed to acquire next swapchain image in BeginFrame.  Error code: " << res << endl;
		return;
	}


	QueueWaitForSemaphore(QueueType::Graphics, presentCompleteSemaphore, 0);

	m_presentCompleteSemaphoreIndex = (m_presentCompleteSemaphoreIndex + 1) % m_presentCompleteSemaphores.size();
}


void DeviceManager::Present()
{ 
	ScopedEvent event{ "DeviceManager::Present" };

	auto renderCompleteSemaphore = m_renderCompleteSemaphores[m_swapChainIndex];

	// Kick the render complete semaphore
	Queue& graphicsQueue = GetQueue(QueueType::Graphics);
	graphicsQueue.AddSignalSemaphore(renderCompleteSemaphore, 0);
	graphicsQueue.AddWaitSemaphore(graphicsQueue.GetTimelineSemaphore(), graphicsQueue.GetLastSubmittedFenceValue());
	graphicsQueue.ExecuteCommandList(VK_NULL_HANDLE, m_presentFences[m_activeFrame]->Get());

	VkSwapchainKHR swapchain = *m_vkSwapChain;

	m_activeFrame = (m_activeFrame + 1) % m_presentFences.size();

	VkSemaphore vkRenderCompleteSemaphore = renderCompleteSemaphore->semaphore->Get();

	VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.pImageIndices = &m_swapChainIndex;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &vkRenderCompleteSemaphore;

	vkQueuePresentKHR(graphicsQueue.GetVkQueue(), &presentInfo);

	ReleaseDeferredResources();

	++m_frameNumber;
}


void DeviceManager::WaitForGpu()
{ 
	for (auto& queue : m_queues)
	{
		queue->WaitForIdle();
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
	instanceBuilder.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT);
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
	//physicalDeviceSelector.require_separate_compute_queue();
	//physicalDeviceSelector.require_separate_transfer_queue();
	auto physicalDeviceRet = physicalDeviceSelector.select();
	if (!physicalDeviceRet)
	{
		LogFatal(LogVulkan) << "Failed to select Vulkan physical device." << endl;
		return;
	}

	m_vkbPhysicalDevice = physicalDeviceRet.value();
	m_vkPhysicalDevice = Create<CVkPhysicalDevice>(m_vkInstance.get(), m_vkbPhysicalDevice.physical_device);	

	// TODO
	m_caps.ReadCaps(*m_vkPhysicalDevice);
	//if (g_graphicsDeviceOptions.logDeviceFeatures)
	if (false)
	{
		m_caps.LogCaps();
	}

	m_limits = std::make_unique<Limits>(m_vkbPhysicalDevice.properties.limits);
	m_deviceName = m_vkbPhysicalDevice.properties.deviceName;

	// Device extensions
	m_extensionManager.InitializeDevice(m_vkbPhysicalDevice);
	SetRequiredDeviceExtensions(m_vkbPhysicalDevice);

	CreateDevice();

	// Create queues
	CreateQueue(QueueType::Graphics);
	CreateQueue(QueueType::Compute);
	CreateQueue(QueueType::Copy);

	// Create the semaphores and fences for present
	const uint32_t numSynchronizationPrimitives = m_desc.numSwapChainBuffers + 1;
	m_presentCompleteSemaphores.reserve(numSynchronizationPrimitives);
	m_renderCompleteSemaphores.reserve(numSynchronizationPrimitives);
	m_presentFences.reserve(numSynchronizationPrimitives);
	for (uint32_t i = 0; i < numSynchronizationPrimitives; ++i)
	{
		// Create present-complete semaphore
		{
			auto semaphore = CreateSemaphore(m_vkDevice.get(), VK_SEMAPHORE_TYPE_BINARY, 0);
			assert(semaphore);
			m_presentCompleteSemaphores.push_back(semaphore);
			string semaphoreName = format("Present Complete Semaphore {}", i);
			semaphore->name = semaphoreName;
			SetDebugName(*m_vkDevice, semaphore->semaphore->Get(), semaphoreName);
		}

		// Create render-complete semaphore
		{
			auto semaphore = CreateSemaphore(m_vkDevice.get(), VK_SEMAPHORE_TYPE_BINARY, 0);
			assert(semaphore);
			m_renderCompleteSemaphores.push_back(semaphore);
			string semaphoreName = format("Render Complete Semaphore {}", i);
			semaphore->name = semaphoreName;
			SetDebugName(*m_vkDevice, semaphore->semaphore->Get(), semaphoreName);
		}

		// Create fence
		{
			auto fence = CreateFence(m_vkDevice.get(), true);
			assert(fence);
			m_presentFences.push_back(fence);
			SetDebugName(*m_vkDevice, fence->Get(), format("Present Fence {}", i));
		}
	}
}


void DeviceManager::CreateWindowSizeDependentResources()
{
	m_swapChainFormat = RemoveSrgb(m_desc.swapChainFormat);
	m_swapChainSurfaceFormat = { FormatToVulkan(m_swapChainFormat), VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

	vkb::SwapchainBuilder swapchainBuilder(m_vkbDevice, m_vkSurface->Get());
	swapchainBuilder.set_desired_extent(m_desc.backBufferWidth, m_desc.backBufferHeight);
	swapchainBuilder.set_desired_format(m_swapChainSurfaceFormat);
	swapchainBuilder.set_desired_present_mode(m_desc.enableVSync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR);

	VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	swapchainBuilder.set_image_usage_flags(usage);
	swapchainBuilder.set_desired_min_image_count(m_desc.numSwapChainBuffers);

	// TODO: Use an optional extension to enable this
	m_swapChainMutableFormatSupported = true;
	if (m_swapChainMutableFormatSupported)
	{
		swapchainBuilder.set_create_flags(VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR);
	}
	swapchainBuilder.set_pre_transform_flags(VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR);
	swapchainBuilder.set_composite_alpha_flags(VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR);
	
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
		swapchainBuilder.add_pNext(&imageFormatListCreateInfo);
	}

	auto swapchainRet = swapchainBuilder.build();
	if (!swapchainRet)
	{
		LogFatal(LogVulkan) << "Failed to create swapchain." << endl;
		return;
	}

	m_vkbSwapchain = swapchainRet.value();

	m_vkSwapChain = Create<CVkSwapchain>(m_vkDevice.get(), m_vkbSwapchain.swapchain);

	vector<VkImage> images = m_vkbSwapchain.get_images().value();

	m_swapChainBuffers.reserve(images.size());
	for (uint32_t i = 0; i < (uint32_t)images.size(); ++i)
	{
		auto cimage = Create<CVkImage>(m_vkDevice.get(), images[i]);
		ColorBufferPtr swapChainBuffer = m_device->CreateColorBufferFromSwapChainImage(cimage.get(), m_desc.backBufferWidth, m_desc.backBufferHeight, m_swapChainFormat, i);
		m_swapChainBuffers.emplace_back(swapChainBuffer);
	}
}


CommandContext* DeviceManager::AllocateContext(CommandListType commandListType)
{
	lock_guard<mutex> lockGuard(m_contextAllocationMutex);

	ScopedEvent event("AllocateContext");

	auto& availableContexts = m_availableContexts[(uint32_t)commandListType];

	CommandContext* ret{ nullptr };
	if (availableContexts.empty())
	{
		wil::com_ptr<ICommandContext> contextImpl = Make<CommandContextVK>(m_vkDevice.get(), commandListType);
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


ColorBufferPtr DeviceManager::GetColorBuffer() const
{
	return m_swapChainBuffers[m_swapChainIndex];
}


Format DeviceManager::GetColorFormat() const
{
	return m_swapChainFormat;
}


Format DeviceManager::GetDepthFormat() const
{
	return m_desc.depthBufferFormat;
}


const string& DeviceManager::GetDeviceName() const
{
	return m_deviceName;
}


IDevice* DeviceManager::GetDevice() 
{ 
	return m_device.get(); 
}


void DeviceManager::ReleaseImage(CVkImage* image)
{
	lock_guard lock(m_deferredReleaseMutex);

	uint64_t nextFence = GetQueue(QueueType::Graphics).GetNextFenceValue();

	DeferredReleaseResource resource{ nextFence, image, nullptr };
	m_deferredResources.emplace_back(resource);
}


void DeviceManager::ReleaseBuffer(CVkBuffer* buffer)
{
	lock_guard lock(m_deferredReleaseMutex);

	uint64_t nextFence = GetQueue(QueueType::Graphics).GetNextFenceValue();

	DeferredReleaseResource resource{ nextFence, nullptr, buffer };
	m_deferredResources.emplace_back(resource);
}


CVkDevice* DeviceManager::GetVulkanDevice() const
{
	return m_vkDevice.get();
}


CVmaAllocator* DeviceManager::GetAllocator() const
{
	return m_vmaAllocator.get();
}


bool DeviceManager::IsDeviceExtensionEnabled(const string& extensionName) const
{
	return m_extensionManager.enabledExtensions.deviceExtensions.contains(extensionName);
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
		"VK_KHR_swapchain_mutable_format"
		//"VK_EXT_descriptor_buffer"
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

	m_device = std::make_unique<Device>(m_vkDevice.get(), m_vmaAllocator.get());

	m_textureManager = std::make_unique<TextureManager>(m_device.get());
}


void DeviceManager::CreateQueue(QueueType queueType)
{
	VkQueue vkQueue{ VK_NULL_HANDLE };
	uint32_t queueIndex{ 0 };

	switch (queueType)
	{
	case QueueType::Graphics:
		queueIndex = m_vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
		vkQueue = m_vkbDevice.get_queue(vkb::QueueType::graphics).value(); 
		break;

	case QueueType::Compute:
	{
		auto queueIndexRes = m_vkbDevice.get_queue_index(vkb::QueueType::compute);
		if (queueIndexRes)
		{
			queueIndex = queueIndexRes.value();
			vkQueue = m_vkbDevice.get_queue(vkb::QueueType::compute).value();

		}
		else
		{
			// Fall back to graphics queue
			queueIndex = m_vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
			vkQueue = m_vkbDevice.get_queue(vkb::QueueType::graphics).value();
		}
	}
		break;

	case QueueType::Copy:
	{
		auto queueIndexRes = m_vkbDevice.get_queue_index(vkb::QueueType::transfer);
		if (queueIndexRes)
		{
			queueIndex = queueIndexRes.value();
			vkQueue = m_vkbDevice.get_queue(vkb::QueueType::transfer).value();

		}
		else
		{
			// Fall back to graphics queue
			queueIndex = m_vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
			vkQueue = m_vkbDevice.get_queue(vkb::QueueType::graphics).value();
		}
	}
		break;

	default:
		queueIndex = m_vkbDevice.get_queue_index(vkb::QueueType::present).value();
		vkQueue = m_vkbDevice.get_queue(vkb::QueueType::present).value();
		break;
	}

	m_queues[(uint32_t)queueType] = make_unique<Queue>(m_vkDevice.get(), vkQueue, queueType, queueIndex);
}


void DeviceManager::ResizeSwapChain()
{
	WaitForGpu();

	m_vkSwapChain.reset();
	m_swapChainBuffers.clear();

	CreateWindowSizeDependentResources();
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


void DeviceManager::QueueWaitForSemaphore(QueueType queueType, SemaphorePtr semaphore, uint64_t value)
{
	m_queues[(uint32_t)queueType]->AddWaitSemaphore(semaphore, value);
}


void DeviceManager::QueueSignalSemaphore(QueueType queueType, SemaphorePtr semaphore, uint64_t value)
{
	m_queues[(uint32_t)queueType]->AddSignalSemaphore(semaphore, value);
}


void DeviceManager::ReleaseDeferredResources()
{
	lock_guard lock(m_deferredReleaseMutex);

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