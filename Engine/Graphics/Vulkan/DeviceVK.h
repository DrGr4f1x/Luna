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

using namespace Microsoft::WRL;


namespace Luna::VK
{

struct GraphicsDeviceDesc
{
	VkInstance instance{ VK_NULL_HANDLE };
	IVkPhysicalDevice* physicalDevice{ nullptr };
	VkDevice device{ VK_NULL_HANDLE };

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
	constexpr GraphicsDeviceDesc& SetPhysicalDevice(IVkPhysicalDevice* value) noexcept { physicalDevice = value; return *this; }
	constexpr GraphicsDeviceDesc& SetDevice(VkDevice value) noexcept { device = value; return *this; }
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


class __declspec(uuid("402B61AE-0C51-46D3-B0EC-A2911B380181")) GraphicsDevice
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, IGraphicsDevice>
	, public NonCopyable
{
public:
	GraphicsDevice(const GraphicsDeviceDesc& desc);
	virtual ~GraphicsDevice();

private:
	GraphicsDeviceDesc m_desc{};
};

} // namespace Luna::VK