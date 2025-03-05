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

#include "Graphics\ColorBuffer.h"
#include "Graphics\DeviceManager.h"
#include "Graphics\Vulkan\DeviceCapsVK.h"
#include "Graphics\Vulkan\ExtensionManagerVK.h"
#include "Graphics\Vulkan\VulkanCommon.h"

using namespace Microsoft::WRL;


namespace Luna::VK
{

// Forward declarations
class ColorBufferManager;
class DepthBufferManager;
class DescriptorSetManager;
class GpuBufferManager;
class PipelineStateManager;
class Queue;
class RootSignatureManager;


class __declspec(uuid("BE54D89A-4FEB-4208-973F-E4B5EBAC4516")) DeviceManager 
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, Luna::IDeviceManager>
	, public NonCopyable
{
	friend class CommandContextVK;

public:
	DeviceManager(const DeviceManagerDesc& desc);
	virtual ~DeviceManager();

	void BeginFrame() final;
	void Present() final;

	void WaitForGpu() final;
	void WaitForFence(uint64_t fenceValue);
	bool IsFenceComplete(uint64_t fenceValue);

	void SetWindowSize(uint32_t width, uint32_t height) final;
	void CreateDeviceResources() final;
	void CreateWindowSizeDependentResources() final;

	CommandContext* AllocateContext(CommandListType commandListType) final;
	void FreeContext(CommandContext* usedContext) final;

	ColorBuffer& GetColorBuffer() final;

	Format GetColorFormat() final;
	Format GetDepthFormat() final;

	IColorBufferManager* GetColorBufferManager() override;
	IDepthBufferManager* GetDepthBufferManager() override;
	IDescriptorSetManager* GetDescriptorSetManager() override;
	IGpuBufferManager* GetGpuBufferManager() override;
	IPipelineStateManager* GetPipelineStateManager() override;
	IRootSignatureManager* GetRootSignatureManager() override;

	void ReleaseImage(CVkImage* image);
	void ReleaseBuffer(CVkBuffer* buffer);

	CVkDevice* GetDevice() const;
	CVmaAllocator* GetAllocator() const;

	// Extensions
	bool IsDeviceExtensionEnabled(const std::string& extensionName) const;

private:
	void SetRequiredInstanceLayersAndExtensions(vkb::InstanceBuilder& instanceBuilder);
	void SetRequiredDeviceExtensions(vkb::PhysicalDevice& physicalDevice);
	void InstallDebugMessenger(vkb::InstanceBuilder& instanceBuilder);

	void CreateSurface();
	void CreateDevice();
	void CreateResourceManagers();
	void CreateQueue(QueueType queueType);

	void ResizeSwapChain();

	void GetQueueFamilyIndices();
	int32_t GetQueueFamilyIndex(VkQueueFlags queueFlags);

	Queue& GetQueue(QueueType queueType);
	Queue& GetQueue(CommandListType commandListType);
	void QueueWaitForSemaphore(QueueType queueType, VkSemaphore semaphore, uint64_t value);
	void QueueSignalSemaphore(QueueType queueType, VkSemaphore, uint64_t value);

	void ReleaseDeferredResources();

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
	wil::com_ptr<CVmaAllocator> m_vmaAllocator;

	// vk-boostrap
	vkb::Instance m_vkbInstance;
	vkb::PhysicalDevice m_vkbPhysicalDevice;
	vkb::Device m_vkbDevice;
	vkb::Swapchain m_vkbSwapchain;

	// Vulkan resource managers
	std::unique_ptr<ColorBufferManager> m_colorBufferManager;
	std::unique_ptr<DepthBufferManager> m_depthBufferManager;
	std::unique_ptr<DescriptorSetManager> m_descriptorSetManager;
	std::unique_ptr<GpuBufferManager> m_gpuBufferManager;
	std::unique_ptr<PipelineStateManager> m_pipelineStateManager;
	std::unique_ptr<RootSignatureManager> m_rootSignatureManager;

	// Swapchain
	wil::com_ptr<CVkSwapchain> m_vkSwapChain;
	// TODO - get rid of this, just use m_swapChainBuffers below
	std::vector<wil::com_ptr<CVkImage>> m_vkSwapChainImages;
	uint32_t m_swapChainIndex{ (uint32_t)-1 };
	bool m_swapChainMutableFormatSupported{ false };
	VkSurfaceFormatKHR m_swapChainSurfaceFormat{};
	Format m_swapChainFormat;

	// Swapchain color buffers
	std::vector<ColorBuffer> m_swapChainBuffers;

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
	std::vector<wil::com_ptr<CVkSemaphore>> m_presentCompleteSemaphores;
	std::vector<wil::com_ptr<CVkSemaphore>> m_renderCompleteSemaphores;
	std::vector<wil::com_ptr<CVkFence>> m_presentFences;
	uint32_t m_activeFrame{ 0 };
	uint32_t m_presentCompleteSemaphoreIndex{ 0 };
	uint32_t m_renderCompleteSemaphoreIndex{ 0 };

	// Command context handling
	std::mutex m_contextAllocationMutex;
	std::vector<std::unique_ptr<CommandContext>> m_contextPool[4];
	std::queue<CommandContext*> m_availableContexts[4];

	// Deferred resource release
	struct DeferredReleaseResource
	{
		uint64_t fenceValue;
		wil::com_ptr<CVkImage> image;
		wil::com_ptr<CVkBuffer> buffer;
	};
	std::list<DeferredReleaseResource> m_deferredResources;
};


DeviceManager* GetVulkanDeviceManager();

} // namespace Luna::VK