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
#include "Graphics\DeviceCaps.h"
#include "Graphics\DeviceManager.h"
#include "Graphics\Texture.h"
#include "Graphics\Vulkan\VulkanCommon.h"

using namespace Microsoft::WRL;


namespace Luna::VK
{

// Forward declarations
class Device;
class Queue;
struct Semaphore;

struct SupportedFeatures 
{
	uint32_t descriptorIndexing : 1;
	uint32_t deviceAddress : 1;
	uint32_t swapChainMutableFormat : 1;
	uint32_t presentId : 1;
	uint32_t memoryPriority : 1;
	uint32_t memoryBudget : 1;
	uint32_t maintenance4 : 1;
	uint32_t maintenance5 : 1;
	uint32_t maintenance6 : 1;
	uint32_t imageSlicedView : 1;
	uint32_t customBorderColor : 1;
	uint32_t robustness : 1;
	uint32_t robustness2 : 1;
	uint32_t pipelineRobustness : 1;
	uint32_t swapChainMaintenance1 : 1;
	uint32_t fifoLatestReady : 1;
	uint32_t descriptorBuffers : 1;
};

static_assert(sizeof(SupportedFeatures) == sizeof(uint32_t), "4 bytes expected");

struct ExtensionFeatures
{
	VkPhysicalDeviceVulkan11Features features11{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
	VkPhysicalDeviceVulkan12Features features12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	VkPhysicalDeviceVulkan13Features features13{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	VkPhysicalDeviceVulkan14Features features14{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES };
	VkPhysicalDeviceSynchronization2Features synchronization2features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES };
	VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES };
	VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT };
	VkPhysicalDeviceMaintenance4Features maintenance4Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES };
	VkPhysicalDeviceImageRobustnessFeatures imageRobustnessFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_ROBUSTNESS_FEATURES };
	VkPhysicalDevicePresentIdFeaturesKHR presentIdFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR };
	VkPhysicalDevicePresentWaitFeaturesKHR presentWaitFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR };
	VkPhysicalDeviceMaintenance5FeaturesKHR maintenance5Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES_KHR };
	VkPhysicalDeviceMaintenance6FeaturesKHR maintenance6Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_6_FEATURES_KHR };
	VkPhysicalDeviceMaintenance7FeaturesKHR maintenance7Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_7_FEATURES_KHR };
	VkPhysicalDeviceMaintenance8FeaturesKHR maintenance8Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_8_FEATURES_KHR };
	VkPhysicalDeviceMaintenance9FeaturesKHR maintenance9Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_9_FEATURES_KHR };
	VkPhysicalDeviceFragmentShadingRateFeaturesKHR shadingRateFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR };
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR };
	VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR };
	VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR };
	VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR rayTracingPositionFetchFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_POSITION_FETCH_FEATURES_KHR };
	VkPhysicalDeviceRayTracingMaintenance1FeaturesKHR rayTracingMaintenanceFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_MAINTENANCE_1_FEATURES_KHR };
	VkPhysicalDeviceLineRasterizationFeaturesKHR lineRasterizationFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_KHR };
	VkPhysicalDeviceFragmentShaderBarycentricFeaturesKHR fragmentShaderBarycentricFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_BARYCENTRIC_FEATURES_KHR };
	VkPhysicalDeviceShaderClockFeaturesKHR shaderClockFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_CLOCK_FEATURES_KHR };
	VkPhysicalDeviceComputeShaderDerivativesFeaturesKHR computeShaderDerivativesFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COMPUTE_SHADER_DERIVATIVES_FEATURES_KHR };
	VkPhysicalDeviceOpacityMicromapFeaturesEXT micromapFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_OPACITY_MICROMAP_FEATURES_EXT };
	VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT };
	VkPhysicalDeviceShaderAtomicFloatFeaturesEXT shaderAtomicFloatFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT };
	VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT shaderAtomicFloat2Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_2_FEATURES_EXT };
	VkPhysicalDeviceMemoryPriorityFeaturesEXT memoryPriorityFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT };
	VkPhysicalDeviceImageSlicedViewOf3DFeaturesEXT slicedViewFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_SLICED_VIEW_OF_3D_FEATURES_EXT };
	VkPhysicalDeviceCustomBorderColorFeaturesEXT borderColorFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT };
	VkPhysicalDeviceRobustness2FeaturesEXT robustness2Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT };
	VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT fragmentShaderInterlockFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT };
	VkPhysicalDeviceSwapchainMaintenance1FeaturesEXT swapchainMaintenance1Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SWAPCHAIN_MAINTENANCE_1_FEATURES_EXT };
	VkPhysicalDevicePresentModeFifoLatestReadyFeaturesEXT presentModeFifoLatestReadyFeaturesEXT{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_MODE_FIFO_LATEST_READY_FEATURES_EXT };
	VkPhysicalDeviceZeroInitializeDeviceMemoryFeaturesEXT zeroInitializeDeviceMemoryFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ZERO_INITIALIZE_DEVICE_MEMORY_FEATURES_EXT };
	VkPhysicalDeviceDescriptorBufferFeaturesEXT descriptorBufferFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT };
};


class DeviceManager : public IDeviceManager, public NonCopyable
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

private:
	void EnableInstanceLayersAndExtensions(vkb::InstanceBuilder& instanceBuilder);
	void EnableDeviceExtensions();
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

	VulkanVersionInfo m_versionInfo{};
	DeviceCaps m_caps{};
	SupportedFeatures m_supportedFeatures{};
	VkPhysicalDeviceMemoryProperties m_memoryProps{};
	VkPhysicalDeviceFeatures2 m_features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
	ExtensionFeatures m_extFeatures{};
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

	// Queues and queue families
	std::array<std::unique_ptr<Queue>, (uint32_t)QueueType::Count> m_queues;

	// Present synchronization
	std::vector<std::shared_ptr<Semaphore>> m_presentCompleteSemaphores;
	std::vector<std::shared_ptr<Semaphore>> m_renderCompleteSemaphores;
	std::vector<wil::com_ptr<CVkFence>> m_presentFences;
	uint32_t m_presentCompleteSemaphoreIndex{ 0 };
	uint64_t m_frameNumber{ 0 };
	uint32_t m_activeFrame{ 0 };

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