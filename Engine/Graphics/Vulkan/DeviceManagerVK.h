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
class GraphicsDevice;


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

	std::vector<std::pair<AdapterInfo, VkPhysicalDevice>> EnumeratePhysicalDevices();
	void GetQueueFamilyIndices();
	int32_t GetQueueFamilyIndex(VkQueueFlags queueFlags);

private:
	DeviceManagerDesc m_desc{};
	bool m_bIsDeveloperModeEnabled{ false };
	bool m_bIsRenderDocAvailable{ false };

	ExtensionManager m_extensionManager;
	VulkanVersionInfo m_versionInfo{};
	DeviceCaps m_caps;

	// Vulkan instance objects owned by the DeviceManager
	wil::com_ptr<IVkInstance> m_vkInstance;
	wil::com_ptr<IVkDebugUtilsMessenger> m_vkDebugMessenger;
	wil::com_ptr<IVkSurface> m_vkSurface;
	wil::com_ptr<IVkPhysicalDevice> m_vkPhysicalDevice;
	wil::com_ptr<IVkDevice> m_vkDevice;

	// Luna objects
	wil::com_ptr<GraphicsDevice> m_device;

	// Queues and queue families
	std::vector<VkQueueFamilyProperties> m_queueFamilyProperties;
	struct
	{
		int32_t graphics{ -1 };
		int32_t compute{ -1 };
		int32_t transfer{ -1 };
		int32_t present{ -1 };
	} m_queueFamilyIndices;
};


DeviceManager* GetVulkanDeviceManager();

} // namespace Luna::VK