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
#include "Graphics\Limits.h"
#include "Graphics\Texture.h"
#include "Graphics\Vulkan\DeviceCapsVK.h"
#include "Graphics\Vulkan\ExtensionManagerVK.h"
#include "Graphics\Vulkan\VulkanCommon.h"

using namespace Microsoft::WRL;


namespace Luna::VK
{

// Forward declarations
class Device;
class Queue;
struct Semaphore;


class Limits : public ILimits
{
public:
	explicit Limits(VkPhysicalDeviceLimits limits);
	~Limits();

	uint32_t ConstantBufferAlignment() const override { return (uint32_t)m_limits.minUniformBufferOffsetAlignment; }
	uint32_t MaxTextureDimension1D() const override { return m_limits.maxImageDimension1D; }
	uint32_t MaxTextureDimension2D() const override { return m_limits.maxImageDimension2D; }
	uint32_t MaxTextureDimension3D() const override { return m_limits.maxImageDimension3D; }
	uint32_t MaxTextureDimensionCube() const override { return m_limits.maxImageDimensionCube; }
	uint32_t MaxTexture1DArrayElements() const override { return m_limits.maxImageArrayLayers; }
	uint32_t MaxTexture2DArrayElements() const override { return m_limits.maxImageArrayLayers;; }
	uint32_t MaxTextureMipLevels() const override { return 15; }

protected:
	VkPhysicalDeviceLimits m_limits;
};


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

	GraphicsApi GetGraphicsApi() const override { return GraphicsApi::Vulkan; }

	ColorBufferPtr GetColorBuffer() const final;
	DepthBufferPtr GetDepthBuffer() const final;

	Format GetColorFormat() const final;
	Format GetDepthFormat() const final;

	const std::string& GetDeviceName() const override;

	uint32_t GetNumSwapChainBuffers() const override { return m_desc.numSwapChainBuffers; }
	uint32_t GetActiveFrame() const override { return m_swapChainIndex; }
	uint64_t GetFrameNumber() const override { return m_frameNumber; }

	IDevice* GetDevice() override;

	void ReleaseImage(CVkImage* image);
	void ReleaseBuffer(CVkBuffer* buffer);

	CVkDevice* GetVulkanDevice() const;
	CVmaAllocator* GetAllocator() const;

	// Extensions
	bool IsDeviceExtensionEnabled(const std::string& extensionName) const;

private:
	void SetRequiredInstanceLayersAndExtensions(vkb::InstanceBuilder& instanceBuilder);
	void SetRequiredDeviceExtensions(vkb::PhysicalDevice& physicalDevice);
	void InstallDebugMessenger(vkb::InstanceBuilder& instanceBuilder);

	void CreateSurface();
	void CreateDevice();
	void CreateQueue(QueueType queueType);

	void ResizeSwapChain();

	Queue& GetQueue(QueueType queueType);
	Queue& GetQueue(CommandListType commandListType);
	void QueueWaitForSemaphore(QueueType queueType, std::shared_ptr<Semaphore> semaphore, uint64_t value);
	void QueueSignalSemaphore(QueueType queueType, std::shared_ptr<Semaphore>, uint64_t value);

	void ReleaseDeferredResources();

private:
	DeviceManagerDesc m_desc{};
	bool m_bIsDeveloperModeEnabled{ false };
	bool m_bIsRenderDocAvailable{ false };

	ExtensionManager m_extensionManager;
	VulkanVersionInfo m_versionInfo{};
	DeviceCaps m_caps;
	std::string m_deviceName;

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

	// Device wrapper
	std::unique_ptr<Device> m_device;

	// Device limits
	std::unique_ptr<Limits> m_limits;

	// Texture manager
	std::unique_ptr<TextureManager> m_textureManager;

	// Swapchain
	wil::com_ptr<CVkSwapchain> m_vkSwapChain;
	uint32_t m_swapChainIndex{ (uint32_t)-1 };
	bool m_swapChainMutableFormatSupported{ false };
	VkSurfaceFormatKHR m_swapChainSurfaceFormat{};
	Format m_swapChainFormat;

	// Swapchain color buffers
	std::vector<ColorBufferPtr> m_swapChainBuffers;

	// Default depth buffer
	DepthBufferPtr m_depthBuffer;

	// Queues and queue families
	std::array<std::unique_ptr<Queue>, (uint32_t)QueueType::Count> m_queues;

	// Present synchronization
	std::vector<std::shared_ptr<Semaphore>> m_presentCompleteSemaphores;
	std::vector<std::shared_ptr<Semaphore>> m_renderCompleteSemaphores;
	uint32_t m_presentCompleteSemaphoreIndex{ 0 };
	uint64_t m_frameNumber{ 0 };

	// Command context handling
	std::mutex m_contextAllocationMutex;
	std::vector<std::unique_ptr<CommandContext>> m_contextPool[4];
	std::queue<CommandContext*> m_availableContexts[4];

	// Deferred resource release
	std::mutex m_deferredReleaseMutex;
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