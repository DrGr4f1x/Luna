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

#include <numeric>

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
		assert(false);
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

#if USE_DESCRIPTOR_BUFFERS
	DescriptorBufferAllocator::DestroyAll();
#endif // USE_DESCRIPTOR_BUFFERS

#if USE_LEGACY_DESCRIPTOR_SETS
	DescriptorSetAllocator::DestroyAll();
#endif // USE_LEGACY_DESCRIPTOR_BUFFERS

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

	// Create Vulkan instance
	vkb::InstanceBuilder instanceBuilder;
	instanceBuilder.set_app_name(m_desc.appName.c_str());
	instanceBuilder.set_engine_name(s_engineName.c_str());
	instanceBuilder.set_engine_version(s_engineVersion);
	instanceBuilder.require_api_version(VK_API_VERSION_1_4);
	instanceBuilder.set_minimum_instance_version(VK_API_VERSION_1_4);
	instanceBuilder.request_validation_layers(m_desc.enableValidation);
	instanceBuilder.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT);
	if (m_desc.enableValidation)
	{
		InstallDebugMessenger(instanceBuilder);
	}

	EnableInstanceLayersAndExtensions(instanceBuilder);

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

	m_deviceName = m_vkbPhysicalDevice.properties.deviceName;

	// Device extensions
	EnableDeviceExtensions();

	// Create logical device
	CreateDevice();

	m_device->GetDeviceCaps().LogCaps();

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

#if USE_DESCRIPTOR_BUFFERS
	// Create user descriptor buffers
	DescriptorBufferAllocator::CreateAll();
#endif
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
		ICommandContext* contextImpl = new CommandContextVK(m_vkDevice.get(), commandListType);
		ret = new CommandContext(contextImpl);

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


void DeviceManager::EnableInstanceLayersAndExtensions(vkb::InstanceBuilder& instanceBuilder)
{
	if (m_desc.enableValidation)
	{
		instanceBuilder.enable_validation_layers(true);
	}

	vector<const char*> requiredExtensions{
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		VK_KHR_SURFACE_EXTENSION_NAME
	};
	instanceBuilder.enable_extensions(requiredExtensions);
}


void DeviceManager::EnableDeviceExtensions()
{
	vector<const char*> requestedExtensions;
	
	auto RequestExtension = [this, &requestedExtensions](const char* extensionName)
		{
			const bool isAvailable = m_vkbPhysicalDevice.is_extension_present(extensionName);
			if (isAvailable)
			{
				requestedExtensions.push_back(extensionName);
			}
			else
			{
				LogWarning(LogVulkan) << "Requested extension " << extensionName << " is not available" << endl;
			}
		};

	auto IsExtensionRequested = [&requestedExtensions](const char* extensionName)
		{
			for (auto& e : requestedExtensions)
			{
				if (!strcmp(extensionName, e))
				{
					return true;
				}
			}
			return false;
		};

	// Mandatory
	if (m_versionInfo.minor < 3) 
	{
		RequestExtension(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
		RequestExtension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
		RequestExtension(VK_KHR_COPY_COMMANDS_2_EXTENSION_NAME);
		RequestExtension(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
	}

	// Optional for Vulkan < 1.3
	if (m_versionInfo.minor < 3)
	{
		RequestExtension(VK_KHR_MAINTENANCE_4_EXTENSION_NAME);
		RequestExtension(VK_EXT_IMAGE_ROBUSTNESS_EXTENSION_NAME);
	}
	
	// Optional (KHR)
	RequestExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	RequestExtension(VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME);
	RequestExtension(VK_KHR_PRESENT_ID_EXTENSION_NAME);
	RequestExtension(VK_KHR_PRESENT_WAIT_EXTENSION_NAME);
	if (m_versionInfo.minor < 4)
	{
		RequestExtension(VK_KHR_MAINTENANCE_5_EXTENSION_NAME);
		RequestExtension(VK_KHR_MAINTENANCE_6_EXTENSION_NAME);
		RequestExtension(VK_KHR_LINE_RASTERIZATION_EXTENSION_NAME);
	}
	RequestExtension(VK_KHR_MAINTENANCE_7_EXTENSION_NAME);
	RequestExtension(VK_KHR_MAINTENANCE_8_EXTENSION_NAME);
	RequestExtension(VK_KHR_MAINTENANCE_9_EXTENSION_NAME);
	RequestExtension(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);
	RequestExtension(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
	RequestExtension(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
	RequestExtension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
	RequestExtension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
	RequestExtension(VK_KHR_RAY_QUERY_EXTENSION_NAME);
	RequestExtension(VK_KHR_RAY_TRACING_POSITION_FETCH_EXTENSION_NAME);
	RequestExtension(VK_KHR_RAY_TRACING_MAINTENANCE_1_EXTENSION_NAME);
	RequestExtension(VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME);
	RequestExtension(VK_KHR_SHADER_CLOCK_EXTENSION_NAME);
	RequestExtension(VK_KHR_COMPUTE_SHADER_DERIVATIVES_EXTENSION_NAME);
	RequestExtension(VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME);
	RequestExtension(VK_EXT_PRESENT_MODE_FIFO_LATEST_READY_EXTENSION_NAME);
	RequestExtension(VK_EXT_OPACITY_MICROMAP_EXTENSION_NAME);
	RequestExtension(VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME);
	RequestExtension(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME);
	RequestExtension(VK_EXT_MESH_SHADER_EXTENSION_NAME);
	RequestExtension(VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME);
	RequestExtension(VK_EXT_SHADER_ATOMIC_FLOAT_2_EXTENSION_NAME);
	RequestExtension(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
	RequestExtension(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME);
	RequestExtension(VK_EXT_IMAGE_SLICED_VIEW_OF_3D_EXTENSION_NAME);
	RequestExtension(VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME);
	RequestExtension(VK_EXT_ROBUSTNESS_2_EXTENSION_NAME);
	RequestExtension(VK_EXT_PIPELINE_ROBUSTNESS_EXTENSION_NAME);
	RequestExtension(VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME);
	RequestExtension(VK_EXT_ZERO_INITIALIZE_DEVICE_MEMORY_EXTENSION_NAME);
	RequestExtension(VK_NV_LOW_LATENCY_2_EXTENSION_NAME);
	RequestExtension(VK_NVX_BINARY_IMPORT_EXTENSION_NAME);
	RequestExtension(VK_NVX_IMAGE_VIEW_HANDLE_EXTENSION_NAME);
	RequestExtension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
	RequestExtension(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME);

	m_vkbPhysicalDevice.enable_extensions_if_present(requestedExtensions);

	// Features
	void** tail = &m_features.pNext;

	VK_APPEND_PNEXT(m_extFeatures.features11);
	VK_APPEND_PNEXT(m_extFeatures.features12);
	if (m_versionInfo.minor >= 3)
	{
		VK_APPEND_PNEXT(m_extFeatures.features13);
	}
	if (m_versionInfo.minor >= 4)
	{
		VK_APPEND_PNEXT(m_extFeatures.features14);
	}

	// Mandatory
	if (IsExtensionRequested(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.synchronization2features);
	}

	if (IsExtensionRequested(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.dynamicRenderingFeatures);
	}

	if (IsExtensionRequested(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.extendedDynamicStateFeatures);
	}

	// Optional (for Vulkan < 1.2)
	if (IsExtensionRequested(VK_KHR_MAINTENANCE_4_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.maintenance4Features);
	}

	if (IsExtensionRequested(VK_EXT_IMAGE_ROBUSTNESS_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.imageRobustnessFeatures);
	}

	// Optional (KHR)
	if (IsExtensionRequested(VK_KHR_PRESENT_ID_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.presentIdFeatures);
	}

	if (IsExtensionRequested(VK_KHR_PRESENT_WAIT_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.presentWaitFeatures);
	}

	if (IsExtensionRequested(VK_KHR_MAINTENANCE_5_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.maintenance5Features);
	}

	if (IsExtensionRequested(VK_KHR_MAINTENANCE_6_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.maintenance6Features);
	}

	if (IsExtensionRequested(VK_KHR_MAINTENANCE_7_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.maintenance7Features);
	}

	if (IsExtensionRequested(VK_KHR_MAINTENANCE_8_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.maintenance8Features);
	}

	if (IsExtensionRequested(VK_KHR_MAINTENANCE_9_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.maintenance9Features);
	}

	if (IsExtensionRequested(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.shadingRateFeatures);
	}

	if (IsExtensionRequested(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.rayTracingPipelineFeatures);
	}

	if (IsExtensionRequested(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.accelerationStructureFeatures);
	}

	if (IsExtensionRequested(VK_KHR_RAY_QUERY_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.rayQueryFeatures);
	}

	if (IsExtensionRequested(VK_KHR_RAY_TRACING_POSITION_FETCH_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.rayTracingPositionFetchFeatures);
	}

	if (IsExtensionRequested(VK_KHR_RAY_TRACING_MAINTENANCE_1_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.rayTracingMaintenanceFeatures);
	}

	if (IsExtensionRequested(VK_KHR_LINE_RASTERIZATION_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.lineRasterizationFeatures);
	}

	if (IsExtensionRequested(VK_KHR_FRAGMENT_SHADER_BARYCENTRIC_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.fragmentShaderBarycentricFeatures);
	}

	if (IsExtensionRequested(VK_KHR_SHADER_CLOCK_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.shaderClockFeatures);
	}

	if (IsExtensionRequested(VK_KHR_COMPUTE_SHADER_DERIVATIVES_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.computeShaderDerivativesFeatures);
	}

	// Optional (EXT)
	if (IsExtensionRequested(VK_EXT_OPACITY_MICROMAP_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.micromapFeatures);
	}

	if (IsExtensionRequested(VK_EXT_MESH_SHADER_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.meshShaderFeatures);
	}

	if (IsExtensionRequested(VK_EXT_SHADER_ATOMIC_FLOAT_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.shaderAtomicFloatFeatures);
	}

	if (IsExtensionRequested(VK_EXT_SHADER_ATOMIC_FLOAT_2_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.shaderAtomicFloat2Features);
	}

	if (IsExtensionRequested(VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.memoryPriorityFeatures);
	}

	if (IsExtensionRequested(VK_EXT_IMAGE_SLICED_VIEW_OF_3D_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.slicedViewFeatures);
	}

	if (IsExtensionRequested(VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.borderColorFeatures);
	}

	if (IsExtensionRequested(VK_EXT_ROBUSTNESS_2_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.robustness2Features);
	}

	if (IsExtensionRequested(VK_EXT_PIPELINE_ROBUSTNESS_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.pipelineRobustnessFeatures);
	}

	if (IsExtensionRequested(VK_EXT_FRAGMENT_SHADER_INTERLOCK_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.fragmentShaderInterlockFeatures);
	}

	if (IsExtensionRequested(VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.swapchainMaintenance1Features);
	}

	if (IsExtensionRequested(VK_EXT_PRESENT_MODE_FIFO_LATEST_READY_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.presentModeFifoLatestReadyFeaturesEXT);
	}

	if (IsExtensionRequested(VK_EXT_ZERO_INITIALIZE_DEVICE_MEMORY_EXTENSION_NAME)) 
	{
		VK_APPEND_PNEXT(m_extFeatures.zeroInitializeDeviceMemoryFeatures);
	}

	if (IsExtensionRequested(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME))
	{
		m_supportedFeatures.memoryBudget = true;
	}

	if (IsExtensionRequested(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME))
	{
		VK_APPEND_PNEXT(m_extFeatures.descriptorBufferFeatures);
	}

	vkGetPhysicalDeviceFeatures2(m_vkPhysicalDevice->Get(), &m_features);

	m_supportedFeatures.descriptorIndexing = m_extFeatures.features12.descriptorIndexing;
	m_supportedFeatures.deviceAddress = m_extFeatures.features12.bufferDeviceAddress;
	m_supportedFeatures.swapChainMutableFormat = IsExtensionRequested(VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME);
	m_supportedFeatures.presentId = m_extFeatures.presentIdFeatures.presentId;
	m_supportedFeatures.memoryPriority = m_extFeatures.memoryPriorityFeatures.memoryPriority;
	m_supportedFeatures.maintenance4 = m_extFeatures.features13.maintenance4 != 0 || m_extFeatures.maintenance4Features.maintenance4 != 0;
	m_supportedFeatures.maintenance5 = m_extFeatures.maintenance5Features.maintenance5;
	m_supportedFeatures.maintenance6 = m_extFeatures.maintenance6Features.maintenance6;
	m_supportedFeatures.imageSlicedView = m_extFeatures.slicedViewFeatures.imageSlicedViewOf3D != 0;
	m_supportedFeatures.customBorderColor = m_extFeatures.borderColorFeatures.customBorderColors != 0 && m_extFeatures.borderColorFeatures.customBorderColorWithoutFormat != 0;
	m_supportedFeatures.robustness = m_features.features.robustBufferAccess != 0 && (m_extFeatures.imageRobustnessFeatures.robustImageAccess != 0 || m_extFeatures.features13.robustImageAccess != 0);
	m_supportedFeatures.robustness2 = m_extFeatures.robustness2Features.robustBufferAccess2 != 0 && m_extFeatures.robustness2Features.robustImageAccess2 != 0;
	m_supportedFeatures.pipelineRobustness = m_extFeatures.pipelineRobustnessFeatures.pipelineRobustness;
	m_supportedFeatures.swapChainMaintenance1 = m_extFeatures.swapchainMaintenance1Features.swapchainMaintenance1;
	m_supportedFeatures.fifoLatestReady = m_extFeatures.presentModeFifoLatestReadyFeaturesEXT.presentModeFifoLatestReady;
	m_supportedFeatures.descriptorBuffers = m_extFeatures.descriptorBufferFeatures.descriptorBuffer;

	// Memory props
	{ 
		VkPhysicalDeviceMemoryProperties2 memoryProps = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2 };
		vkGetPhysicalDeviceMemoryProperties2(m_vkPhysicalDevice->Get(), &memoryProps);

		m_memoryProps = memoryProps.memoryProperties;
	}

	// Fill caps
	{
		m_caps.api = GraphicsApi::Vulkan;

		// Device properties
		VkPhysicalDeviceProperties2 props = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
		void** tail = &props.pNext;

		VkPhysicalDeviceVulkan11Properties props11 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES };
		VK_APPEND_PNEXT(props11);

		VkPhysicalDeviceVulkan12Properties props12 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES };
		VK_APPEND_PNEXT(props12);

		VkPhysicalDeviceVulkan13Properties props13 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES };
		if (m_versionInfo.minor >= 3)
		{
			VK_APPEND_PNEXT(props13);
		}

		VkPhysicalDeviceVulkan14Properties props14 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_PROPERTIES };
		if (m_versionInfo.minor >= 4)
		{
			VK_APPEND_PNEXT(props14);
		}

		VkPhysicalDeviceSubgroupSizeControlProperties subgroupSizeControlProps = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES };
		if (m_versionInfo.minor < 3)
		{
			VK_APPEND_PNEXT(subgroupSizeControlProps);
		}

		VkPhysicalDeviceMaintenance4PropertiesKHR maintenance4Props = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_PROPERTIES_KHR };
		if (m_versionInfo.minor < 3 && IsExtensionRequested(VK_KHR_MAINTENANCE_4_EXTENSION_NAME))
		{
			VK_APPEND_PNEXT(maintenance4Props);
		}

		VkPhysicalDeviceMaintenance5PropertiesKHR maintenance5Props = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_PROPERTIES_KHR };
		if (IsExtensionRequested(VK_KHR_MAINTENANCE_5_EXTENSION_NAME)) 
		{
			VK_APPEND_PNEXT(maintenance5Props);
		}

		VkPhysicalDeviceMaintenance6PropertiesKHR maintenance6Props = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_6_PROPERTIES_KHR };
		if (IsExtensionRequested(VK_KHR_MAINTENANCE_6_EXTENSION_NAME)) 
		{
			VK_APPEND_PNEXT(maintenance6Props);
		}

		VkPhysicalDeviceMaintenance7PropertiesKHR maintenance7Props = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_7_PROPERTIES_KHR };
		if (IsExtensionRequested(VK_KHR_MAINTENANCE_7_EXTENSION_NAME))
		{
			VK_APPEND_PNEXT(maintenance7Props);
		}

		VkPhysicalDeviceMaintenance9PropertiesKHR maintenance9Props = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_9_PROPERTIES_KHR };
		if (IsExtensionRequested(VK_KHR_MAINTENANCE_9_EXTENSION_NAME)) 
		{
			VK_APPEND_PNEXT(maintenance9Props);
		}

		VkPhysicalDeviceLineRasterizationPropertiesKHR lineRasterizationProps = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_PROPERTIES_KHR };
		if (IsExtensionRequested(VK_KHR_LINE_RASTERIZATION_EXTENSION_NAME)) 
		{
			VK_APPEND_PNEXT(lineRasterizationProps);
		}

		VkPhysicalDeviceFragmentShadingRatePropertiesKHR shadingRateProps = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR };
		if (IsExtensionRequested(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME))
		{
			VK_APPEND_PNEXT(shadingRateProps);
		}

		VkPhysicalDevicePushDescriptorPropertiesKHR pushDescriptorProps = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR };
		if (IsExtensionRequested(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME)) 
		{
			VK_APPEND_PNEXT(pushDescriptorProps);
		}

		VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProps = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
		if (IsExtensionRequested(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)) 
		{
			VK_APPEND_PNEXT(rayTracingProps);
		}

		VkPhysicalDeviceAccelerationStructurePropertiesKHR accelerationStructureProps = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR };
		if (IsExtensionRequested(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)) 
		{
			VK_APPEND_PNEXT(accelerationStructureProps);
		}

		VkPhysicalDeviceConservativeRasterizationPropertiesEXT conservativeRasterProps = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT };
		if (IsExtensionRequested(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME)) 
		{
			VK_APPEND_PNEXT(conservativeRasterProps);
			m_caps.tiers.conservativeRaster = 1;
		}

		VkPhysicalDeviceSampleLocationsPropertiesEXT sampleLocationsProps = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLE_LOCATIONS_PROPERTIES_EXT };
		if (IsExtensionRequested(VK_EXT_SAMPLE_LOCATIONS_EXTENSION_NAME)) 
		{
			VK_APPEND_PNEXT(sampleLocationsProps);
			m_caps.tiers.sampleLocations = 1;
		}

		VkPhysicalDeviceMeshShaderPropertiesEXT meshShaderProps = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT };
		if (IsExtensionRequested(VK_EXT_MESH_SHADER_EXTENSION_NAME)) 
		{
			VK_APPEND_PNEXT(meshShaderProps);
		}

		VkPhysicalDeviceOpacityMicromapPropertiesEXT micromapProps = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPACITY_MICROMAP_PROPERTIES_EXT };
		if (IsExtensionRequested(VK_EXT_OPACITY_MICROMAP_EXTENSION_NAME)) 
		{
			VK_APPEND_PNEXT(micromapProps);
		}

		VkPhysicalDeviceComputeShaderDerivativesPropertiesKHR computeShaderDerivativesProps = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_PROPERTIES_KHR };
		if (IsExtensionRequested(VK_KHR_COMPUTE_SHADER_DERIVATIVES_EXTENSION_NAME)) 
		{
			VK_APPEND_PNEXT(computeShaderDerivativesProps);
		}

		VkPhysicalDeviceDescriptorBufferPropertiesEXT descriptorBufferProps = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT };
		if (IsExtensionRequested(VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME))
		{
			VK_APPEND_PNEXT(descriptorBufferProps);
		}

		vkGetPhysicalDeviceProperties2(m_vkPhysicalDevice->Get(), &props);

		const VkPhysicalDeviceLimits& limits = props.properties.limits;

		m_caps.viewport.maxNum = limits.maxViewports;
		m_caps.viewport.boundsMin = (int16_t)limits.viewportBoundsRange[0];
		m_caps.viewport.boundsMax = (int16_t)limits.viewportBoundsRange[1];

		m_caps.dimensions.attachmentMaxDim = (uint16_t)std::min(limits.maxFramebufferWidth, limits.maxFramebufferHeight);
		m_caps.dimensions.attachmentLayerMaxNum = (uint16_t)limits.maxFramebufferLayers;
		m_caps.dimensions.texture1DMaxDim = (uint16_t)limits.maxImageDimension1D;
		m_caps.dimensions.texture2DMaxDim = (uint16_t)limits.maxImageDimension2D;
		m_caps.dimensions.texture3DMaxDim = (uint16_t)limits.maxImageDimension3D;
		m_caps.dimensions.textureLayerMaxNum = (uint16_t)limits.maxImageArrayLayers;
		m_caps.dimensions.textureCubeMaxDim = (uint16_t)limits.maxImageDimensionCube;
		m_caps.dimensions.typedBufferMaxDim = limits.maxTexelBufferElements;

		m_caps.precision.viewportBits = limits.viewportSubPixelBits;
		m_caps.precision.subPixelBits = limits.subPixelPrecisionBits;
		m_caps.precision.subTexelBits = limits.subTexelPrecisionBits;
		m_caps.precision.mipmapBits = limits.mipmapPrecisionBits;

		const VkMemoryPropertyFlags neededFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		for (uint32_t i = 0; i < m_memoryProps.memoryTypeCount; i++) 
		{
			const VkMemoryType& memoryType = m_memoryProps.memoryTypes[i];
			if ((memoryType.propertyFlags & neededFlags) == neededFlags)
			{
				m_caps.memory.deviceUploadHeapSize += m_memoryProps.memoryHeaps[memoryType.heapIndex].size;
			}
		}

		m_caps.memory.allocationMaxNum = limits.maxMemoryAllocationCount;
		m_caps.memory.samplerAllocationMaxNum = limits.maxSamplerAllocationCount;
		m_caps.memory.constantBufferMaxRange = limits.maxUniformBufferRange;
		m_caps.memory.storageBufferMaxRange = limits.maxStorageBufferRange;
		m_caps.memory.bufferTextureGranularity = (uint32_t)limits.bufferImageGranularity;
		m_caps.memory.bufferMaxSize = m_versionInfo.minor >= 3 ? props13.maxBufferSize : maintenance4Props.maxBufferSize;

		// VUID-VkCopyBufferToImageInfo2-dstImage-07975: If "dstImage" does not have either a depth/stencil format or a multi-planar format,
		//      "bufferOffset" must be a multiple of the texel block size
		// VUID-VkCopyBufferToImageInfo2-dstImage-07978: If "dstImage" has a depth/stencil format,
		//      "bufferOffset" must be a multiple of 4
		// Least Common Multiple stride across all formats: 1, 2, 4, 8, 16 // TODO: rarely used "12" fucks up the beauty of power-of-2 numbers, such formats must be avoided!
		constexpr uint32_t leastCommonMultipleStrideAccrossAllFormats = 16;

		m_caps.memoryAlignment.uploadBufferTextureRow = (uint32_t)limits.optimalBufferCopyRowPitchAlignment;
		m_caps.memoryAlignment.uploadBufferTextureSlice = std::lcm((uint32_t)limits.optimalBufferCopyOffsetAlignment, leastCommonMultipleStrideAccrossAllFormats);
		m_caps.memoryAlignment.shaderBindingTable = rayTracingProps.shaderGroupBaseAlignment;
		m_caps.memoryAlignment.bufferShaderResourceOffset = std::lcm((uint32_t)limits.minTexelBufferOffsetAlignment, (uint32_t)limits.minStorageBufferOffsetAlignment);
		m_caps.memoryAlignment.constantBufferOffset = (uint32_t)limits.minUniformBufferOffsetAlignment;
		m_caps.memoryAlignment.scratchBufferOffset = accelerationStructureProps.minAccelerationStructureScratchOffsetAlignment;
		m_caps.memoryAlignment.accelerationStructureOffset = 256; // see the spec
		m_caps.memoryAlignment.micromapOffset = 256;              // see the spec

		m_caps.pipelineLayout.descriptorSetMaxNum = limits.maxBoundDescriptorSets;
		m_caps.pipelineLayout.rootConstantMaxSize = limits.maxPushConstantsSize;
		m_caps.pipelineLayout.rootDescriptorMaxNum = pushDescriptorProps.maxPushDescriptors;

		m_caps.descriptorSet.samplerMaxNum = limits.maxDescriptorSetSamplers;
		m_caps.descriptorSet.constantBufferMaxNum = limits.maxDescriptorSetUniformBuffers;
		m_caps.descriptorSet.storageBufferMaxNum = limits.maxDescriptorSetStorageBuffers;
		m_caps.descriptorSet.textureMaxNum = limits.maxDescriptorSetSampledImages;
		m_caps.descriptorSet.storageTextureMaxNum = limits.maxDescriptorSetStorageImages;

		m_caps.descriptorSet.updateAfterSet.samplerMaxNum = props12.maxDescriptorSetUpdateAfterBindSamplers;
		m_caps.descriptorSet.updateAfterSet.constantBufferMaxNum = props12.maxDescriptorSetUpdateAfterBindUniformBuffers;
		m_caps.descriptorSet.updateAfterSet.storageBufferMaxNum = props12.maxDescriptorSetUpdateAfterBindStorageBuffers;
		m_caps.descriptorSet.updateAfterSet.textureMaxNum = props12.maxDescriptorSetUpdateAfterBindSampledImages;
		m_caps.descriptorSet.updateAfterSet.storageTextureMaxNum = props12.maxDescriptorSetUpdateAfterBindStorageImages;

		m_caps.descriptorBuffer.combinedImageSamplerDescriptorSingleArray = descriptorBufferProps.combinedImageSamplerDescriptorSingleArray == VK_TRUE;
		m_caps.descriptorBuffer.bufferlessPushDescriptors = descriptorBufferProps.bufferlessPushDescriptors == VK_TRUE;
		m_caps.descriptorBuffer.allowSamplerImageViewPostSubmitCreation = descriptorBufferProps.allowSamplerImageViewPostSubmitCreation == VK_TRUE;
		m_caps.descriptorBuffer.descriptorBufferOffsetAlignment = descriptorBufferProps.descriptorBufferOffsetAlignment;
		m_caps.descriptorBuffer.maxDescriptorBufferBindings = descriptorBufferProps.maxDescriptorBufferBindings;
		m_caps.descriptorBuffer.maxResourceDescriptorBufferBindings = descriptorBufferProps.maxResourceDescriptorBufferBindings;
		m_caps.descriptorBuffer.maxSamplerDescriptorBufferBindings = descriptorBufferProps.maxSamplerDescriptorBufferBindings;
		m_caps.descriptorBuffer.maxEmbeddedImmutableSamplerBindings = descriptorBufferProps.maxEmbeddedImmutableSamplerBindings;
		m_caps.descriptorBuffer.maxEmbeddedImmutableSamplers = descriptorBufferProps.maxEmbeddedImmutableSamplers;
		m_caps.descriptorBuffer.maxSamplerDescriptorBufferRange = descriptorBufferProps.maxSamplerDescriptorBufferRange;
		m_caps.descriptorBuffer.maxResourceDescriptorBufferRange = descriptorBufferProps.maxResourceDescriptorBufferRange;
		m_caps.descriptorBuffer.samplerDescriptorBufferAddressSpaceSize = descriptorBufferProps.samplerDescriptorBufferAddressSpaceSize;
		m_caps.descriptorBuffer.resourceDescriptorBufferAddressSpaceSize = descriptorBufferProps.resourceDescriptorBufferAddressSpaceSize;
		m_caps.descriptorBuffer.descriptorBufferAddressSpaceSize = descriptorBufferProps.descriptorBufferAddressSpaceSize;

		m_caps.descriptorBuffer.descriptorSize.sampler = descriptorBufferProps.samplerDescriptorSize;
		m_caps.descriptorBuffer.descriptorSize.combinedImageSampler = descriptorBufferProps.combinedImageSamplerDescriptorSize;
		m_caps.descriptorBuffer.descriptorSize.sampledImage = descriptorBufferProps.sampledImageDescriptorSize;
		m_caps.descriptorBuffer.descriptorSize.storageImage = descriptorBufferProps.storageImageDescriptorSize;
		m_caps.descriptorBuffer.descriptorSize.uniformTexelBuffer = descriptorBufferProps.uniformTexelBufferDescriptorSize;
		m_caps.descriptorBuffer.descriptorSize.robustUniformTexelBuffer = descriptorBufferProps.robustUniformTexelBufferDescriptorSize;
		m_caps.descriptorBuffer.descriptorSize.storageTexelBuffer = descriptorBufferProps.storageTexelBufferDescriptorSize;
		m_caps.descriptorBuffer.descriptorSize.robustStorageTexelBuffer = descriptorBufferProps.robustStorageTexelBufferDescriptorSize;
		m_caps.descriptorBuffer.descriptorSize.uniformBuffer = descriptorBufferProps.uniformBufferDescriptorSize;
		m_caps.descriptorBuffer.descriptorSize.robustUniformBuffer = descriptorBufferProps.robustUniformBufferDescriptorSize;
		m_caps.descriptorBuffer.descriptorSize.storageBuffer = descriptorBufferProps.storageBufferDescriptorSize;
		m_caps.descriptorBuffer.descriptorSize.robustStorageBuffer = descriptorBufferProps.robustStorageBufferDescriptorSize;
		m_caps.descriptorBuffer.descriptorSize.inputAttachment = descriptorBufferProps.inputAttachmentDescriptorSize;
		m_caps.descriptorBuffer.descriptorSize.accelerationStructure = descriptorBufferProps.accelerationStructureDescriptorSize;

		m_caps.shaderStage.descriptorSamplerMaxNum = limits.maxPerStageDescriptorSamplers;
		m_caps.shaderStage.descriptorConstantBufferMaxNum = limits.maxPerStageDescriptorUniformBuffers;
		m_caps.shaderStage.descriptorStorageBufferMaxNum = limits.maxPerStageDescriptorStorageBuffers;
		m_caps.shaderStage.descriptorTextureMaxNum = limits.maxPerStageDescriptorSampledImages;
		m_caps.shaderStage.descriptorStorageTextureMaxNum = limits.maxPerStageDescriptorStorageImages;
		m_caps.shaderStage.resourceMaxNum = limits.maxPerStageResources;

		m_caps.shaderStage.updateAfterSet.descriptorSamplerMaxNum = props12.maxPerStageDescriptorUpdateAfterBindSamplers;
		m_caps.shaderStage.updateAfterSet.descriptorConstantBufferMaxNum = props12.maxPerStageDescriptorUpdateAfterBindUniformBuffers;
		m_caps.shaderStage.updateAfterSet.descriptorStorageBufferMaxNum = props12.maxPerStageDescriptorUpdateAfterBindStorageBuffers;
		m_caps.shaderStage.updateAfterSet.descriptorTextureMaxNum = props12.maxPerStageDescriptorUpdateAfterBindSampledImages;
		m_caps.shaderStage.updateAfterSet.descriptorStorageTextureMaxNum = props12.maxPerStageDescriptorUpdateAfterBindStorageImages;
		m_caps.shaderStage.updateAfterSet.resourceMaxNum = props12.maxPerStageUpdateAfterBindResources;

		m_caps.shaderStage.vertex.attributeMaxNum = limits.maxVertexInputAttributes;
		m_caps.shaderStage.vertex.streamMaxNum = limits.maxVertexInputBindings;
		m_caps.shaderStage.vertex.outputComponentMaxNum = limits.maxVertexOutputComponents;

		m_caps.shaderStage.hull.generationMaxLevel = (float)limits.maxTessellationGenerationLevel;
		m_caps.shaderStage.hull.patchPointMaxNum = limits.maxTessellationPatchSize;
		m_caps.shaderStage.hull.perVertexInputComponentMaxNum = limits.maxTessellationControlPerVertexInputComponents;
		m_caps.shaderStage.hull.perVertexOutputComponentMaxNum = limits.maxTessellationControlPerVertexOutputComponents;
		m_caps.shaderStage.hull.perPatchOutputComponentMaxNum = limits.maxTessellationControlPerPatchOutputComponents;
		m_caps.shaderStage.hull.totalOutputComponentMaxNum = limits.maxTessellationControlTotalOutputComponents;

		m_caps.shaderStage.domain.inputComponentMaxNum = limits.maxTessellationEvaluationInputComponents;
		m_caps.shaderStage.domain.outputComponentMaxNum = limits.maxTessellationEvaluationOutputComponents;

		m_caps.shaderStage.geometry.invocationMaxNum = limits.maxGeometryShaderInvocations;
		m_caps.shaderStage.geometry.inputComponentMaxNum = limits.maxGeometryInputComponents;
		m_caps.shaderStage.geometry.outputComponentMaxNum = limits.maxGeometryOutputComponents;
		m_caps.shaderStage.geometry.outputVertexMaxNum = limits.maxGeometryOutputVertices;
		m_caps.shaderStage.geometry.totalOutputComponentMaxNum = limits.maxGeometryTotalOutputComponents;

		m_caps.shaderStage.pixel.inputComponentMaxNum = limits.maxFragmentInputComponents;
		m_caps.shaderStage.pixel.attachmentMaxNum = limits.maxFragmentOutputAttachments;
		m_caps.shaderStage.pixel.dualSourceAttachmentMaxNum = limits.maxFragmentDualSrcAttachments;

		m_caps.shaderStage.compute.workGroupMaxNum[0] = limits.maxComputeWorkGroupCount[0];
		m_caps.shaderStage.compute.workGroupMaxNum[1] = limits.maxComputeWorkGroupCount[1];
		m_caps.shaderStage.compute.workGroupMaxNum[2] = limits.maxComputeWorkGroupCount[2];
		m_caps.shaderStage.compute.workGroupMaxDim[0] = limits.maxComputeWorkGroupSize[0];
		m_caps.shaderStage.compute.workGroupMaxDim[1] = limits.maxComputeWorkGroupSize[1];
		m_caps.shaderStage.compute.workGroupMaxDim[2] = limits.maxComputeWorkGroupSize[2];
		m_caps.shaderStage.compute.workGroupInvocationMaxNum = limits.maxComputeWorkGroupInvocations;
		m_caps.shaderStage.compute.sharedMemoryMaxSize = limits.maxComputeSharedMemorySize;

		m_caps.shaderStage.rayTracing.shaderGroupIdentifierSize = rayTracingProps.shaderGroupHandleSize;
		m_caps.shaderStage.rayTracing.tableMaxStride = rayTracingProps.maxShaderGroupStride;
		m_caps.shaderStage.rayTracing.recursionMaxDepth = rayTracingProps.maxRayRecursionDepth;

		m_caps.shaderStage.meshControl.sharedMemoryMaxSize = meshShaderProps.maxTaskSharedMemorySize;
		m_caps.shaderStage.meshControl.workGroupInvocationMaxNum = meshShaderProps.maxTaskWorkGroupInvocations;
		m_caps.shaderStage.meshControl.payloadMaxSize = meshShaderProps.maxTaskPayloadSize;

		m_caps.shaderStage.meshEvaluation.outputVerticesMaxNum = meshShaderProps.maxMeshOutputVertices;
		m_caps.shaderStage.meshEvaluation.outputPrimitiveMaxNum = meshShaderProps.maxMeshOutputPrimitives;
		m_caps.shaderStage.meshEvaluation.outputComponentMaxNum = meshShaderProps.maxMeshOutputComponents;
		m_caps.shaderStage.meshEvaluation.sharedMemoryMaxSize = meshShaderProps.maxMeshSharedMemorySize;
		m_caps.shaderStage.meshEvaluation.workGroupInvocationMaxNum = meshShaderProps.maxMeshWorkGroupInvocations;

		m_caps.wave.laneMinNum = m_versionInfo.minor >= 3 ? props13.minSubgroupSize : subgroupSizeControlProps.minSubgroupSize;
		m_caps.wave.laneMaxNum = m_versionInfo.minor >= 3 ? props13.maxSubgroupSize : subgroupSizeControlProps.minSubgroupSize;

		m_caps.wave.derivativeOpsStages = ShaderStage::Pixel;
		if (m_extFeatures.computeShaderDerivativesFeatures.computeDerivativeGroupQuads || m_extFeatures.computeShaderDerivativesFeatures.computeDerivativeGroupLinear)
		{
			m_caps.wave.derivativeOpsStages |= ShaderStage::Compute;
		}
		if (computeShaderDerivativesProps.meshAndTaskShaderDerivatives)
		{
			m_caps.wave.derivativeOpsStages |= ShaderStage::Amplification | ShaderStage::Mesh;
		}

		if (props11.subgroupSupportedOperations & VK_SUBGROUP_FEATURE_QUAD_BIT)
		{
			m_caps.wave.quadOpsStages = props11.subgroupQuadOperationsInAllStages ? ShaderStage::All : (ShaderStage::Pixel | ShaderStage::Compute);
		}

		if (props11.subgroupSupportedStages & VK_SHADER_STAGE_VERTEX_BIT)
		{
			m_caps.wave.waveOpsStages |= ShaderStage::Vertex;
		}
		if (props11.subgroupSupportedStages & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT)
		{
			m_caps.wave.waveOpsStages |= ShaderStage::Hull;
		}
		if (props11.subgroupSupportedStages & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
		{
			m_caps.wave.waveOpsStages |= ShaderStage::Domain;
		}
		if (props11.subgroupSupportedStages & VK_SHADER_STAGE_GEOMETRY_BIT)
		{
			m_caps.wave.waveOpsStages |= ShaderStage::Geometry;
		}
		if (props11.subgroupSupportedStages & VK_SHADER_STAGE_FRAGMENT_BIT)
		{
			m_caps.wave.waveOpsStages |= ShaderStage::Pixel;
		}
		if (props11.subgroupSupportedStages & VK_SHADER_STAGE_COMPUTE_BIT)
		{
			m_caps.wave.waveOpsStages |= ShaderStage::Compute;
		}
		if (props11.subgroupSupportedStages & VK_SHADER_STAGE_RAYGEN_BIT_KHR)
		{
			m_caps.wave.waveOpsStages |= ShaderStage::RayGeneration;
		}
		if (props11.subgroupSupportedStages & VK_SHADER_STAGE_ANY_HIT_BIT_KHR)
		{
			m_caps.wave.waveOpsStages |= ShaderStage::AnyHit;
		}
		if (props11.subgroupSupportedStages & VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
		{
			m_caps.wave.waveOpsStages |= ShaderStage::ClosestHit;
		}
		if (props11.subgroupSupportedStages & VK_SHADER_STAGE_MISS_BIT_KHR)
		{
			m_caps.wave.waveOpsStages |= ShaderStage::Miss;
		}
		if (props11.subgroupSupportedStages & VK_SHADER_STAGE_INTERSECTION_BIT_KHR)
		{
			m_caps.wave.waveOpsStages |= ShaderStage::Intersection;
		}
		if (props11.subgroupSupportedStages & VK_SHADER_STAGE_CALLABLE_BIT_KHR)
		{
			m_caps.wave.waveOpsStages |= ShaderStage::Callable;
		}
		if (props11.subgroupSupportedStages & VK_SHADER_STAGE_TASK_BIT_EXT)
		{
			m_caps.wave.waveOpsStages |= ShaderStage::Amplification;
		}
		if (props11.subgroupSupportedStages & VK_SHADER_STAGE_MESH_BIT_EXT)
		{
			m_caps.wave.waveOpsStages |= ShaderStage::Mesh;
		}

		m_caps.other.timestampFrequencyHz = uint64_t(1e9 / double(limits.timestampPeriod) + 0.5);
		m_caps.other.micromapSubdivisionMaxLevel = micromapProps.maxOpacity2StateSubdivisionLevel;
		m_caps.other.drawIndirectMaxNum = limits.maxDrawIndirectCount;
		m_caps.other.samplerLodBiasMax = limits.maxSamplerLodBias;
		m_caps.other.samplerAnisotropyMax = limits.maxSamplerAnisotropy;
		m_caps.other.texelOffsetMin = (int8_t)limits.minTexelOffset;
		m_caps.other.texelOffsetMax = (uint8_t)limits.maxTexelOffset;
		m_caps.other.texelGatherOffsetMin = (int8_t)limits.minTexelGatherOffset;
		m_caps.other.texelGatherOffsetMax = (uint8_t)limits.maxTexelGatherOffset;
		m_caps.other.clipDistanceMaxNum = (uint8_t)limits.maxClipDistances;
		m_caps.other.cullDistanceMaxNum = (uint8_t)limits.maxCullDistances;
		m_caps.other.combinedClipAndCullDistanceMaxNum = (uint8_t)limits.maxCombinedClipAndCullDistances;
		m_caps.other.viewMaxNum = m_extFeatures.features11.multiview ? (uint8_t)props11.maxMultiviewViewCount : 1;
		m_caps.other.shadingRateAttachmentTileSize = (uint8_t)shadingRateProps.minFragmentShadingRateAttachmentTexelSize.width;

		if (m_caps.tiers.conservativeRaster) 
		{
			if (conservativeRasterProps.primitiveOverestimationSize < 1.0f / 2.0f && conservativeRasterProps.degenerateTrianglesRasterized)
			{
				m_caps.tiers.conservativeRaster = 2;
			}
			if (conservativeRasterProps.primitiveOverestimationSize <= 1.0 / 256.0f && conservativeRasterProps.degenerateTrianglesRasterized)
			{
				m_caps.tiers.conservativeRaster = 3;
			}
		}

		if (m_caps.tiers.sampleLocations) 
		{
			constexpr VkSampleCountFlags allSampleCounts = VK_SAMPLE_COUNT_1_BIT | VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT | VK_SAMPLE_COUNT_16_BIT;
			if (sampleLocationsProps.sampleLocationSampleCounts == allSampleCounts) // like in D3D12 spec
			{
				m_caps.tiers.sampleLocations = 2;
			}
		}

		m_caps.tiers.rayTracing = m_extFeatures.accelerationStructureFeatures.accelerationStructure != 0;
		if (m_caps.tiers.rayTracing) 
		{
			if (m_extFeatures.rayTracingPipelineFeatures.rayTracingPipelineTraceRaysIndirect && m_extFeatures.rayQueryFeatures.rayQuery)
			{
				m_caps.tiers.rayTracing++;
			}
			if (m_extFeatures.micromapFeatures.micromap)
			{
				m_caps.tiers.rayTracing++;
			}
		}

		m_caps.tiers.shadingRate = m_extFeatures.shadingRateFeatures.pipelineFragmentShadingRate != 0;
		if (m_caps.tiers.shadingRate) 
		{
			if (m_extFeatures.shadingRateFeatures.primitiveFragmentShadingRate && m_extFeatures.shadingRateFeatures.attachmentFragmentShadingRate)
			{
				m_caps.tiers.shadingRate = 2;
			}

			m_caps.features.additionalShadingRates = shadingRateProps.maxFragmentSize.height > 2 || shadingRateProps.maxFragmentSize.width > 2;
		}

		m_caps.tiers.bindless = m_supportedFeatures.descriptorIndexing ? 1 : 0;
		m_caps.tiers.resourceBinding = 2; // TODO: seems to be the best match
		m_caps.tiers.memory = 1;          // TODO: seems to be the best match

		m_caps.features.getMemoryDesc2 = m_supportedFeatures.maintenance4;
		m_caps.features.enhancedBarriers = true;
		m_caps.features.swapChain = IsExtensionRequested(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		m_caps.features.rayTracing = m_caps.tiers.rayTracing != 0;
		m_caps.features.meshShader = m_extFeatures.meshShaderFeatures.meshShader != 0 && m_extFeatures.meshShaderFeatures.taskShader != 0;
		m_caps.features.lowLatency = m_supportedFeatures.presentId != 0 && IsExtensionRequested(VK_NV_LOW_LATENCY_2_EXTENSION_NAME);
		m_caps.features.micromap = m_extFeatures.micromapFeatures.micromap != 0;

		m_caps.features.independentFrontAndBackStencilReferenceAndMasks = true;
		m_caps.features.textureFilterMinMax = m_extFeatures.features12.samplerFilterMinmax;
		m_caps.features.logicOp = m_features.features.logicOp;
		m_caps.features.depthBoundsTest = m_features.features.depthBounds;
		m_caps.features.drawIndirectCount = m_extFeatures.features12.drawIndirectCount;
		m_caps.features.lineSmoothing = m_extFeatures.lineRasterizationFeatures.smoothLines;
		m_caps.features.copyQueueTimestamp = limits.timestampComputeAndGraphics;
		m_caps.features.meshShaderPipelineStats = m_extFeatures.meshShaderFeatures.meshShaderQueries == VK_TRUE;
		m_caps.features.dynamicDepthBias = true;
		m_caps.features.viewportOriginBottomLeft = true;
		m_caps.features.regionResolve = true;
		m_caps.features.layerBasedMultiview = m_extFeatures.features11.multiview;
		m_caps.features.presentFromCompute = true;
		m_caps.features.waitableSwapChain = m_extFeatures.presentIdFeatures.presentId != 0 && m_extFeatures.presentWaitFeatures.presentWait != 0;
		m_caps.features.pipelineStatistics = m_features.features.pipelineStatisticsQuery;

		m_caps.shaderFeatures.nativeI16 = m_features.features.shaderInt16;
		m_caps.shaderFeatures.nativeF16 = m_extFeatures.features12.shaderFloat16;
		m_caps.shaderFeatures.nativeI64 = m_features.features.shaderInt64;
		m_caps.shaderFeatures.nativeF64 = m_features.features.shaderFloat64;
		m_caps.shaderFeatures.atomicsF16 = (m_extFeatures.shaderAtomicFloat2Features.shaderBufferFloat16Atomics || m_extFeatures.shaderAtomicFloat2Features.shaderSharedFloat16Atomics) ? true : false;
		m_caps.shaderFeatures.atomicsF32 = (m_extFeatures.shaderAtomicFloatFeatures.shaderBufferFloat32Atomics || m_extFeatures.shaderAtomicFloatFeatures.shaderSharedFloat32Atomics) ? true : false;
		m_caps.shaderFeatures.atomicsI64 = (m_extFeatures.features12.shaderBufferInt64Atomics || m_extFeatures.features12.shaderSharedInt64Atomics) ? true : false;
		m_caps.shaderFeatures.atomicsF64 = (m_extFeatures.shaderAtomicFloatFeatures.shaderBufferFloat64Atomics || m_extFeatures.shaderAtomicFloatFeatures.shaderSharedFloat64Atomics) ? true : false;
		m_caps.shaderFeatures.viewportIndex = m_extFeatures.features12.shaderOutputViewportIndex;
		m_caps.shaderFeatures.layerIndex = m_extFeatures.features12.shaderOutputLayer;
		m_caps.shaderFeatures.clock = (m_extFeatures.shaderClockFeatures.shaderDeviceClock || m_extFeatures.shaderClockFeatures.shaderSubgroupClock) ? true : false;
		m_caps.shaderFeatures.rasterizedOrderedView = m_extFeatures.fragmentShaderInterlockFeatures.fragmentShaderPixelInterlock != 0 && m_extFeatures.fragmentShaderInterlockFeatures.fragmentShaderSampleInterlock != 0;
		m_caps.shaderFeatures.barycentric = m_extFeatures.fragmentShaderBarycentricFeatures.fragmentShaderBarycentric;
		m_caps.shaderFeatures.rayTracingPositionFetch = m_extFeatures.rayTracingPositionFetchFeatures.rayTracingPositionFetch;
		m_caps.shaderFeatures.storageReadWithoutFormat = m_features.features.shaderStorageImageReadWithoutFormat;
		m_caps.shaderFeatures.storageWriteWithoutFormat = m_features.features.shaderStorageImageWriteWithoutFormat;
		m_caps.shaderFeatures.waveQuery = (props11.subgroupSupportedOperations & VK_SUBGROUP_FEATURE_BASIC_BIT) ? true : false;
		m_caps.shaderFeatures.waveVote = (props11.subgroupSupportedOperations & VK_SUBGROUP_FEATURE_VOTE_BIT) ? true : false;
		m_caps.shaderFeatures.waveShuffle = (props11.subgroupSupportedOperations & VK_SUBGROUP_FEATURE_SHUFFLE_BIT) ? true : false;
		m_caps.shaderFeatures.waveArithmetic = (props11.subgroupSupportedOperations & VK_SUBGROUP_FEATURE_ARITHMETIC_BIT) ? true : false;
		m_caps.shaderFeatures.waveReduction = (props11.subgroupSupportedOperations & VK_SUBGROUP_FEATURE_BALLOT_BIT) ? true : false;
		m_caps.shaderFeatures.waveQuad = (props11.subgroupSupportedOperations & VK_SUBGROUP_FEATURE_QUAD_BIT) ? true : false;

		// Estimate shader model last since it depends on many "m_caps" fields
		// Based on https://docs.vulkan.org/guide/latest/hlsl.html#_shader_model_coverage // TODO: code below needs to be improved
		m_caps.shaderModel = 51;
		if (m_caps.shaderFeatures.nativeI64)
			m_caps.shaderModel = 60;
		if (m_caps.other.viewMaxNum > 1 || m_caps.shaderFeatures.barycentric)
			m_caps.shaderModel = 61;
		if (m_caps.shaderFeatures.nativeF16 || m_caps.shaderFeatures.nativeI16)
			m_caps.shaderModel = 62;
		if (m_caps.features.rayTracing)
			m_caps.shaderModel = 63;
		if (m_caps.tiers.shadingRate >= 2)
			m_caps.shaderModel = 64;
		if (m_caps.features.meshShader || m_caps.tiers.rayTracing >= 2)
			m_caps.shaderModel = 65;
		if (m_caps.shaderFeatures.atomicsI64)
			m_caps.shaderModel = 66;
		if (m_features.features.shaderStorageImageMultisample)
			m_caps.shaderModel = 67;
		// TODO: add SM 6.8 and 6.9 detection
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

	deviceBuilder.add_pNext(&m_features);

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
	allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

	VmaAllocator vmaAllocator{ VK_NULL_HANDLE };
	if (VK_SUCCEEDED(vmaCreateAllocator(&allocatorCreateInfo, &vmaAllocator)))
	{
		m_vmaAllocator = Create<CVmaAllocator>(m_vkDevice.get(), vmaAllocator);
	}
	else
	{
		LogError(LogVulkan) << "Failed to create VmaAllocator.  Error code: " << res << endl;
	}

	m_device = std::make_unique<Device>(m_vkDevice.get(), m_vmaAllocator.get(), m_caps);

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