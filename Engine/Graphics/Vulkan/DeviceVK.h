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

#include "Graphics\GraphicsDevice.h"
#include "Graphics\Vulkan\VulkanCommon.h"
#include "Graphics\Vulkan\DescriptorSetPoolVK.h"
#include "Graphics\Vulkan\PipelineStatePoolVK.h"
#include "Graphics\Vulkan\RootSignaturePoolVK.h"

using namespace Microsoft::WRL;


// Forward declarations
namespace Luna
{

class GraphicsPSOData;
class Shader;

} // namespace Luna


namespace Luna::VK
{

struct GraphicsDeviceDesc
{
	VkInstance instance{ VK_NULL_HANDLE };
	CVkPhysicalDevice* physicalDevice{ nullptr };
	CVkDevice* device{ nullptr };

	struct {
		int32_t graphics{ -1 };
		int32_t compute{ -1 };
		int32_t transfer{ -1 };
		int32_t present{ -1 };
	} queueFamilyIndices{};

	uint32_t backBufferWidth{ 0 };
	uint32_t backBufferHeight{ 0 };
	uint32_t numSwapChainBuffers{ 3 };
	Format swapChainFormat{ Format::Unknown };
	VkSurfaceKHR surface{ VK_NULL_HANDLE };

	bool enableVSync{ false };
	uint32_t maxFramesInFlight{ 2 };

#if ENABLE_VULKAN_VALIDATION
	bool enableValidation{ true };
#else
	bool enableValidation{ false };
#endif

#if ENABLE_VULKAN_DEBUG_MARKERS
	bool enableDebugMarkers{ true };
#else
	bool enableDebugMarkers{ false };
#endif

	constexpr GraphicsDeviceDesc& SetInstance(VkInstance value) noexcept { instance = value; return *this; }
	constexpr GraphicsDeviceDesc& SetPhysicalDevice(CVkPhysicalDevice* value) noexcept { physicalDevice = value; return *this; }
	constexpr GraphicsDeviceDesc& SetDevice(CVkDevice* value) noexcept { device = value; return *this; }
	constexpr GraphicsDeviceDesc& SetGraphicsQueueIndex(int32_t value) noexcept { queueFamilyIndices.graphics = value; return *this; }
	constexpr GraphicsDeviceDesc& SetComputeQueueIndex(int32_t value) noexcept { queueFamilyIndices.compute = value; return *this; }
	constexpr GraphicsDeviceDesc& SetTransferQueueIndex(int32_t value) noexcept { queueFamilyIndices.transfer = value; return *this; }
	constexpr GraphicsDeviceDesc& SetPresentQueueIndex(int32_t value) noexcept { queueFamilyIndices.present = value; return *this; }
	constexpr GraphicsDeviceDesc& SetBackBufferWidth(uint32_t value) noexcept { backBufferWidth = value; return *this; }
	constexpr GraphicsDeviceDesc& SetBackBufferHeight(uint32_t value) noexcept { backBufferHeight = value; return *this; }
	constexpr GraphicsDeviceDesc& SetNumSwapChainBuffers(uint32_t value) noexcept { numSwapChainBuffers = value; return *this; }
	constexpr GraphicsDeviceDesc& SetSwapChainFormat(Format value) noexcept { swapChainFormat = value; return *this; }
	constexpr GraphicsDeviceDesc& SetSurface(VkSurfaceKHR value) noexcept { surface = value; return *this; }
	constexpr GraphicsDeviceDesc& SetEnableVSync(bool value) noexcept { enableVSync = value; return *this; }
	constexpr GraphicsDeviceDesc& SetMaxFramesInFlight(uint32_t value) noexcept { maxFramesInFlight = value; return *this; }
	constexpr GraphicsDeviceDesc& SetEnableValidation(bool value) noexcept { enableValidation = value; return *this; }
	constexpr GraphicsDeviceDesc& SetEnableDebugMarkers(bool value) noexcept { enableDebugMarkers = value; return *this; }
};


struct ImageDesc
{
	std::string name;
	uint64_t width{ 0 };
	uint32_t height{ 0 };
	uint32_t arraySizeOrDepth{ 1 };
	Format format{ Format::Unknown };
	uint32_t numMips{ 1 };
	uint32_t numSamples{ 1 };
	ResourceType resourceType{ ResourceType::Unknown };
	GpuImageUsage imageUsage{ GpuImageUsage::Unknown };
	MemoryAccess memoryAccess{ MemoryAccess::Unknown };

	ImageDesc& SetName(const std::string& value) { name = value; return *this; }
	constexpr ImageDesc& SetWidth(uint64_t value) noexcept { width = value; return *this; }
	constexpr ImageDesc& SetHeight(uint32_t value) noexcept { height = value; return *this; }
	constexpr ImageDesc& SetArraySize(uint32_t value) noexcept { arraySizeOrDepth = value; return *this; }
	constexpr ImageDesc& SetDepth(uint32_t value) noexcept { arraySizeOrDepth = value; return *this; }
	constexpr ImageDesc& SetFormat(Format value) noexcept { format = value; return *this; }
	constexpr ImageDesc& SetNumMips(uint32_t value) noexcept { numMips = value; return *this; }
	constexpr ImageDesc& SetNumSamples(uint32_t value) noexcept { numSamples = value; return *this; }
	constexpr ImageDesc& SetResourceType(ResourceType value) noexcept { resourceType = value; return *this; }
	constexpr ImageDesc& SetImageUsage(GpuImageUsage value) noexcept { imageUsage = value; return *this; }
	constexpr ImageDesc& SetMemoryAccess(MemoryAccess value) noexcept { memoryAccess = value; return *this; }
};


struct ImageViewDesc
{
	CVkImage* image{ nullptr };
	std::string name;
	ResourceType resourceType{ ResourceType::Unknown };
	GpuImageUsage imageUsage{ GpuImageUsage::Unknown };
	Format format{ Format::Unknown };
	ImageAspect imageAspect{ 0 };
	TextureSubresourceViewType viewType{ TextureSubresourceViewType::AllAspects };
	uint32_t baseMipLevel{ 0 };
	uint32_t mipCount{ 1 };
	uint32_t baseArraySlice{ 0 };
	uint32_t arraySize{ 1 };

	constexpr ImageViewDesc& SetImage(CVkImage* value) noexcept { image = value; return *this; }
	ImageViewDesc& SetName(const std::string& value) { name = value; return *this; }
	constexpr ImageViewDesc& SetResourceType(ResourceType value) noexcept { resourceType = value; return *this; }
	constexpr ImageViewDesc& SetImageUsage(GpuImageUsage value) noexcept { imageUsage = value; return *this; }
	constexpr ImageViewDesc& SetFormat(Format value) noexcept { format = value; return *this; }
	constexpr ImageViewDesc& SetImageAspect(ImageAspect value) noexcept { imageAspect = value; return *this; }
	constexpr ImageViewDesc& SetViewType(TextureSubresourceViewType value) noexcept { viewType = value; return *this; }
	constexpr ImageViewDesc& SetBaseMipLevel(uint32_t value) noexcept { baseMipLevel = value; return *this; }
	constexpr ImageViewDesc& SetMipCount(uint32_t value) noexcept { mipCount = value; return *this; }
	constexpr ImageViewDesc& SetBaseArraySlice(uint32_t value) noexcept { baseArraySlice = value; return *this; }
	constexpr ImageViewDesc& SetArraySize(uint32_t value) noexcept { arraySize = value; return *this; }
};


class __declspec(uuid("402B61AE-0C51-46D3-B0EC-A2911B380181")) GraphicsDevice
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, IGraphicsDevice>
	, public NonCopyable
{
	friend class CommandContextVK;
	friend class DeviceManager;
	friend class Queue;
	friend class PipelineStatePool;

public:
	GraphicsDevice(const GraphicsDeviceDesc& desc);
	virtual ~GraphicsDevice();

	// GraphicsDevice implementation
	wil::com_ptr<IColorBuffer> CreateColorBuffer(const ColorBufferDesc& colorBufferDesc) override;
	wil::com_ptr<IDepthBuffer> CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc) override;
	wil::com_ptr<IGpuBuffer> CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc) override;

	RootSignatureHandle CreateRootSignature(const RootSignatureDesc& rootSignatureDesc) override;
	PipelineStateHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& graphicsPipelineDesc) override;

	IDescriptorSetPool* GetDescriptorSetPool() override { return &m_descriptorSetPool; }
	IPipelineStatePool* GetPipelineStatePool() override { return &m_pipelinePool; }
	IRootSignaturePool* GetRootSignaturePool() override { return &m_rootSignaturePool; }

	void CreateResources();

	VkDevice GetDevice() noexcept { return *m_vkDevice; }
	VmaAllocator GetAllocator() noexcept { return *m_vmaAllocator; }

private:
	wil::com_ptr<CVkFence> CreateFence(bool bSignalled) const;
	wil::com_ptr<CVkSemaphore> CreateSemaphore(VkSemaphoreType semaphoreType, uint64_t initialValue) const;
	wil::com_ptr<CVkCommandPool> CreateCommandPool(CommandListType commandListType) const;
	wil::com_ptr<CVmaAllocator> CreateVmaAllocator() const;
	wil::com_ptr<CVkImage> CreateImage(const ImageDesc& desc) const;
	wil::com_ptr<CVkImageView> CreateImageView(const ImageViewDesc& desc) const;
	
	wil::com_ptr<CVkBuffer> CreateStagingBuffer(const void* initialData, size_t numBytes) const;

	VkFormatProperties GetFormatProperties(Format format) const;

private:
	GraphicsDeviceDesc m_desc{};

	wil::com_ptr<CVkDevice> m_vkDevice;

	// VmaAllocator
	wil::com_ptr<CVmaAllocator> m_vmaAllocator;

	// Platform data pools
	DescriptorSetPool m_descriptorSetPool;
	PipelineStatePool m_pipelinePool;
	RootSignaturePool m_rootSignaturePool;
};

} // namespace Luna::VK