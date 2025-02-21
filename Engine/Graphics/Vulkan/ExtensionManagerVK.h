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

#include "Graphics\Vulkan\VulkanCommon.h"

#include <unordered_set>


namespace Luna::VK
{

struct ExtensionSet
{
	std::unordered_set<std::string> instanceExtensions;
	std::unordered_set<std::string> instanceLayers;
	std::unordered_set<std::string> deviceExtensions;
};


class ExtensionManager
{
public:
	bool InitializeSystem();
	bool InitializeDevice(vkb::PhysicalDevice& physicalDevice);

	bool EnableInstanceLayers(vkb::InstanceBuilder& instanceBuilder);
	bool EnableInstanceExtensions(vkb::InstanceBuilder& instanceBuilder);
	bool EnableDeviceExtensions(vkb::PhysicalDevice& physicalDevice);;

	void SetRequiredInstanceLayer(const std::string& layer) { requiredExtensions.instanceLayers.insert(layer); m_instanceLayersValidated = false; }
	void SetRequiredInstanceLayers(const std::vector<std::string>& layers) { requiredExtensions.instanceLayers.insert(layers.begin(), layers.end()); m_instanceLayersValidated = false; }
	void SetRequiredInstanceExtension(const std::string& extension) { requiredExtensions.instanceExtensions.insert(extension); m_instanceExtensionsValidated = false; }
	void SetRequiredInstanceExtensions(const std::vector<std::string>& extensions) { requiredExtensions.instanceExtensions.insert(extensions.begin(), extensions.end()); m_instanceExtensionsValidated = false; }
	void SetRequiredDeviceExtension(const std::string& extension) { requiredExtensions.deviceExtensions.insert(extension); m_deviceExtensionsValidated = false; }
	void SetRequiredDeviceExtensions(const std::vector<std::string>& extensions) { requiredExtensions.deviceExtensions.insert(extensions.begin(), extensions.end()); m_deviceExtensionsValidated = false; }

	void SetOptionalInstanceLayer(const std::string& layer) { optionalExtensions.instanceLayers.insert(layer); }
	void SetOptionalInstanceLayers(const std::vector<std::string>& layers) { optionalExtensions.instanceLayers.insert(layers.begin(), layers.end()); }
	void SetOptionalInstanceExtension(const std::string& extension) { optionalExtensions.instanceExtensions.insert(extension); }
	void SetOptionalInstanceExtensions(const std::vector<std::string>& extensions) { optionalExtensions.instanceExtensions.insert(extensions.begin(), extensions.end()); }
	void SetOptionalDeviceExtension(const std::string& extension) { optionalExtensions.deviceExtensions.insert(extension); }
	void SetOptionalDeviceExtensions(const std::vector<std::string>& extensions) { optionalExtensions.deviceExtensions.insert(extensions.begin(), extensions.end()); }

public:
	ExtensionSet availableExtensions;
	ExtensionSet enabledExtensions;

	ExtensionSet requiredExtensions;
	ExtensionSet optionalExtensions;

private:
	bool ValidateInstanceLayers();
	bool ValidateInstanceExtensions();
	bool ValidateDeviceExtensions();

private:
	bool m_instanceLayersValidated{ false };
	bool m_instanceExtensionsValidated{ false };
	bool m_deviceExtensionsValidated{ false };
};

} // namespace Luna::VK