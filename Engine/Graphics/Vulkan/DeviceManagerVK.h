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

#include "Graphics\GraphicsDevice.h"
#include "Graphics\Vulkan\DeviceCapsVK.h"
#include "Graphics\Vulkan\ExtensionManagerVK.h"

using namespace Microsoft::WRL;


namespace Luna::VK
{

// Forward declarations
class ColorBuffer;
class GraphicsDevice;
class Queue;


class __declspec(uuid("BE54D89A-4FEB-4208-973F-E4B5EBAC4516")) DeviceManager 
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, Luna::IDeviceManager>
	, public NonCopyable
{
public:
	DeviceManager(const DeviceManagerDesc& desc);
	virtual ~DeviceManager();

	void BeginFrame() final;
	void Present() final;

	void WaitForGpu() final;

	void CreateDeviceResources() final;
	void CreateWindowSizeDependentResources() final;

	ICommandContext* AllocateContext(CommandListType commandListType) final;

	IColorBuffer* GetColorBuffer() const final;

private:
	void SetRequiredInstanceLayersAndExtensions();
	void InstallDebugMessenger();

	void CreateSurface();
	void SelectPhysicalDevice();
	void CreateDevice();
	void CreateQueue(QueueType queueType);

	wil::com_ptr<ColorBuffer> CreateColorBufferFromSwapChain(uint32_t imageIndex);

	std::vector<std::pair<AdapterInfo, VkPhysicalDevice>> EnumeratePhysicalDevices();
	void GetQueueFamilyIndices();
	int32_t GetQueueFamilyIndex(VkQueueFlags queueFlags);

	Queue& GetQueue(QueueType queueType);
	Queue& GetQueue(CommandListType commandListType);
	void QueueWaitForSemaphore(QueueType queueType, VkSemaphore semaphore, uint64_t value);
	void QueueSignalSemaphore(QueueType queueType, VkSemaphore, uint64_t value);

private:
	DeviceManagerDesc m_desc{};
	bool m_bIsDeveloperModeEnabled{ false };
	bool m_bIsRenderDocAvailable{ false };

	ExtensionManager m_extensionManager;
	VulkanVersionInfo m_versionInfo{};
	DeviceCaps m_caps;

	// Vulkan instance objects owned by the DeviceManager
	wil::com_ptr<CVkInstance> m_vkInstance;
	wil::com_ptr<CVkDebugUtilsMessenger> m_vkDebugMessenger;
	wil::com_ptr<CVkSurface> m_vkSurface;
	wil::com_ptr<CVkPhysicalDevice> m_vkPhysicalDevice;
	wil::com_ptr<CVkDevice> m_vkDevice;

	// Luna objects
	wil::com_ptr<GraphicsDevice> m_device;

	// Swapchain
	wil::com_ptr<CVkSwapchain> m_vkSwapChain;
	// TODO - get rid of this, just use m_swapChainBuffers below
	std::vector<wil::com_ptr<CVkImage>> m_vkSwapChainImages;
	uint32_t m_swapChainIndex{ (uint32_t)-1 };
	bool m_swapChainMutableFormatSupported{ false };
	VkSurfaceFormatKHR m_swapChainFormat{};

	// Swapchain color buffers
	std::vector<wil::com_ptr<ColorBuffer>> m_swapChainBuffers;

	// Queues and queue families
	std::vector<VkQueueFamilyProperties> m_queueFamilyProperties;
	struct
	{
		int32_t graphics{ -1 };
		int32_t compute{ -1 };
		int32_t transfer{ -1 };
		int32_t present{ -1 };
	} m_queueFamilyIndices;
	std::array<std::unique_ptr<Queue>, (uint32_t)QueueType::Count> m_queues;

	// Present synchronization
	std::vector<wil::com_ptr<CVkSemaphore>> m_presentSemaphores;
	std::vector<wil::com_ptr<CVkFence>> m_presentFences;
	std::vector<uint32_t> m_presentFenceState;
	uint32_t m_presentSemaphoreIndex{ 0 };
};


DeviceManager* GetVulkanDeviceManager();

} // namespace Luna::VK