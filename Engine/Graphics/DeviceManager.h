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
#include "Graphics\Enums.h"
#include "Graphics\Formats.h"


namespace Luna
{

// Forward declarations
class CommandContext;
class IColorBufferManager;
class IDepthBufferManager;
class IDescriptorSetManager;
class IGpuBufferManager;
class IPipelineStateManager;
class IRootSignaturePool;


struct DeviceManagerDesc
{
	std::string appName{};
	GraphicsApi graphicsApi{ GraphicsApi::D3D12 };
	bool enableValidation{ false };
	bool enableDebugMarkers{ false };
	bool logDeviceCaps{ true };
	bool allowSoftwareDevice{ false };
	bool preferDiscreteDevice{ true };

	// TODO - set these values from ApplicationInfo
	bool startMaximized{ false };
	bool startFullscreen{ false };
	bool allowModeSwitch{ true };
	int32_t windowPosX{ -1 };
	int32_t windowPosY{ -1 };
	uint32_t backBufferWidth{ 1920 };
	uint32_t backBufferHeight{ 1080 };
	uint32_t refreshRate{ 0 };
	bool enableVSync{ false };

	uint32_t numSwapChainBuffers{ 3 };
	Format swapChainFormat{ Format::SRGBA8_UNorm };
	Format depthBufferFormat{ Format::D32S8 };
	uint32_t swapChainSampleCount{ 1 };
	uint32_t swapChainSampleQuality{ 0 };
	uint32_t maxFramesInFlight{ 2 };
	bool enablePerMonitorDPI{ false };
	bool allowHDROutput{ false };

	HWND hwnd{ nullptr };
	HINSTANCE hinstance{ nullptr };

	// Setters
	DeviceManagerDesc& SetAppName(const std::string& value) { appName = value; return *this; }
	constexpr DeviceManagerDesc& SetGraphicsApi(GraphicsApi value) noexcept { graphicsApi = value; return *this; }
	constexpr DeviceManagerDesc& SetEnableValidation(bool value) noexcept { enableValidation = value; return *this; }
	constexpr DeviceManagerDesc& SetEnableDebugMarkers(bool value) noexcept { enableDebugMarkers = value; return *this; }
	constexpr DeviceManagerDesc& SetLogDeviceCaps(bool value) noexcept { logDeviceCaps = value; return *this; }
	constexpr DeviceManagerDesc& SetAllowSoftwareDevice(bool value) noexcept { allowSoftwareDevice = value; return *this; }
	constexpr DeviceManagerDesc& SetPreferDiscreteDevice(bool value) noexcept { preferDiscreteDevice = value; return *this; }
	constexpr DeviceManagerDesc& SetStartMaximized(bool value) noexcept { startMaximized = value; return *this; }
	constexpr DeviceManagerDesc& SetStartFullscreen(bool value) noexcept { startFullscreen = value; return *this; }
	constexpr DeviceManagerDesc& SetAllowModeSwitch(bool value) noexcept { allowModeSwitch = value; return *this; }
	constexpr DeviceManagerDesc& SetWindowPosX(uint32_t value) noexcept { windowPosX = value; return *this; }
	constexpr DeviceManagerDesc& SetWindowPosY(uint32_t value) noexcept { windowPosY = value; return *this; }
	constexpr DeviceManagerDesc& SetBackBufferWidth(uint32_t value) noexcept { backBufferWidth = value; return *this; }
	constexpr DeviceManagerDesc& SetBackBufferHeight(uint32_t value) noexcept { backBufferHeight = value; return *this; }
	constexpr DeviceManagerDesc& SetRefreshRate(uint32_t value) noexcept { refreshRate = value; return *this; }
	constexpr DeviceManagerDesc& SetEnableVSync(bool value) noexcept { enableVSync = value; return *this; }
	constexpr DeviceManagerDesc& SetNumSwapChainBuffers(uint32_t value) noexcept { numSwapChainBuffers = value; return *this; }
	constexpr DeviceManagerDesc& SetSwapChainFormat(Format value) noexcept { swapChainFormat = value; return *this; }
	constexpr DeviceManagerDesc& SetDepthBufferFormat(Format value) noexcept { depthBufferFormat = value; return *this; }
	constexpr DeviceManagerDesc& SetSwapChainSampleCount(uint32_t value) noexcept { swapChainSampleCount = value; return *this; }
	constexpr DeviceManagerDesc& SetSwapChainSampleQuality(uint32_t value) noexcept { swapChainSampleQuality = value; return *this; }
	constexpr DeviceManagerDesc& SetMaxFramesInFlight(uint32_t value) noexcept { maxFramesInFlight = value; return *this; }
	constexpr DeviceManagerDesc& SetEnablePerMonitorDPI(bool value) noexcept { enablePerMonitorDPI = value; return *this; }
	constexpr DeviceManagerDesc& SetAllowHDROutput(bool value) noexcept { allowHDROutput = value; return *this; }
	constexpr DeviceManagerDesc& SetHwnd(HWND value) noexcept { hwnd = value; return *this; }
	constexpr DeviceManagerDesc& SetHinstance(HINSTANCE value) noexcept { hinstance = value; return *this; }
};


class __declspec(uuid("000FE461-B46B-43D2-803F-19CE5291525A")) IDeviceManager : public IUnknown
{
public:
	virtual ~IDeviceManager() = default;

	virtual void BeginFrame() = 0;
	virtual void Present() = 0;

	virtual void WaitForGpu() = 0;

	virtual void SetWindowSize(uint32_t width, uint32_t height) = 0;
	virtual void CreateDeviceResources() = 0;
	virtual void CreateWindowSizeDependentResources() = 0;

	virtual CommandContext* AllocateContext(CommandListType commandListType) = 0;
	virtual void FreeContext(CommandContext* usedContext) = 0;

	virtual ColorBuffer& GetColorBuffer() = 0;

	virtual Format GetColorFormat() = 0;
	virtual Format GetDepthFormat() = 0;

	virtual IColorBufferManager* GetColorBufferManager() = 0;
	virtual IDepthBufferManager* GetDepthBufferManager() = 0;
	virtual IDescriptorSetManager* GetDescriptorSetManager() = 0;
	virtual IGpuBufferManager* GetGpuBufferManager() = 0;
	virtual IPipelineStateManager* GetPipelineStateManager() = 0;
	virtual IRootSignaturePool* GetRootSignaturePool() = 0;
};

} // namespace Luna