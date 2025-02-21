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

#include "ExtensionManagerVK.h"

using namespace std;


namespace Luna::VK
{

bool ExtensionManager::InitializeSystem()
{
	auto systemInfoRet = vkb::SystemInfo::get_system_info();
	if (!systemInfoRet)
	{
		return false;
	}

	auto systemInfo = systemInfoRet.value();

	// Instance layers
	LogInfo(LogVulkan) << "Enumerating available Vulkan instance layers" << endl;
	availableExtensions.instanceLayers.clear();
	for (const auto& layer : systemInfo.available_layers)
	{
		LogInfo(LogVulkan) << "  - " << layer.layerName << endl;
		availableExtensions.instanceLayers.insert(layer.layerName);
	}

	// Instance extensions
	LogInfo(LogVulkan) << "Enumerating available Vulkan instance extensions" << endl;
	availableExtensions.instanceExtensions.clear();
	for (const auto& ext : systemInfo.available_extensions)
	{
		LogInfo(LogVulkan) << "  - " << ext.extensionName << endl;
		availableExtensions.instanceExtensions.insert(ext.extensionName);
	}

	return true;
}


bool ExtensionManager::InitializeDevice(vkb::PhysicalDevice& physicalDevice)
{
	LogInfo(LogVulkan) << "Enumerating available Vulkan device extensions" << endl;

	availableExtensions.deviceExtensions.clear();
	for (const auto& extension : physicalDevice.get_available_extensions())
	{
		LogInfo(LogVulkan) << "  - " << extension << endl;
		availableExtensions.deviceExtensions.insert(extension);
	}

	return true;
}


bool ExtensionManager::EnableInstanceLayers(vkb::InstanceBuilder& instanceBuilder)
{
	if (!m_instanceLayersValidated)
	{
		if (!ValidateInstanceLayers())
		{
			return false;
		}
	}

	for (const auto& layer : enabledExtensions.instanceLayers)
	{
		instanceBuilder.enable_layer(layer.c_str());
	}

	return true;
}


bool ExtensionManager::EnableInstanceExtensions(vkb::InstanceBuilder& instanceBuilder)
{
	if (!m_instanceExtensionsValidated)
	{
		if (!ValidateInstanceExtensions())
		{
			return false;
		}
	}

	for (const auto& extension : enabledExtensions.instanceExtensions)
	{
		instanceBuilder.enable_extension(extension.c_str());
	}

	return true;
}


bool ExtensionManager::EnableDeviceExtensions(vkb::PhysicalDevice& physicalDevice)
{
	if (!m_deviceExtensionsValidated)
	{
		if (!ValidateDeviceExtensions())
		{
			return false;
		}
	}

	for (const auto& extension : enabledExtensions.deviceExtensions)
	{
		physicalDevice.enable_extension_if_present(extension.c_str());
	}

	return true;
}


bool ExtensionManager::ValidateInstanceLayers()
{
	// Check required layers
	vector<string> missingLayers;
	for (const auto& layer : requiredExtensions.instanceLayers)
	{
		if (availableExtensions.instanceLayers.find(layer) != availableExtensions.instanceLayers.end())
		{
			enabledExtensions.instanceLayers.insert(layer);
		}
		else
		{
			missingLayers.push_back(layer);
		}
	}

	if (!missingLayers.empty())
	{
		LogError(LogVulkan) << "Failed to create Vulkan instance because the following required layer(s) are not supported:" << endl;
		for (const auto& layer : missingLayers)
		{
			LogError(LogVulkan) << "  - " << layer << endl;
		}
		return false;
	}

	// Check optional layers
	missingLayers.clear();
	for (const auto& layer : optionalExtensions.instanceLayers)
	{
		if (availableExtensions.instanceLayers.find(layer) != availableExtensions.instanceLayers.end())
		{
			enabledExtensions.instanceLayers.insert(layer);
		}
		else
		{
			missingLayers.push_back(layer);
		}
	}

	if (!missingLayers.empty())
	{
		LogWarning(LogVulkan) << "The following optional Vulkan instance layer(s) are not supported:" << endl;
		for (const auto& layer : missingLayers)
		{
			LogWarning(LogVulkan) << " - " << layer << endl;
		}
	}

	LogInfo(LogVulkan) << "Enabling Vulkan instance layers:" << endl;
	for (const auto& layer : enabledExtensions.instanceLayers)
	{
		LogInfo(LogVulkan) << "  - " << layer << endl;
	}

	m_instanceLayersValidated = true;

	return true;
}

bool ExtensionManager::ValidateInstanceExtensions()
{
	// Check required extensions
	vector<string> missingExtensions;
	for (const auto& extension : requiredExtensions.instanceExtensions)
	{
		if (availableExtensions.instanceExtensions.find(extension) != availableExtensions.instanceExtensions.end())
		{
			enabledExtensions.instanceExtensions.insert(extension);
		}
		else
		{
			missingExtensions.push_back(extension);
		}
	}

	if (!missingExtensions.empty())
	{
		LogError(LogVulkan) << "Failed to create Vulkan instance because the following required extensions(s) are not supported:" << endl;
		for (const auto& extension : missingExtensions)
		{
			LogError(LogVulkan) << "  - " << extension << endl;
		}
		return false;
	}

	// Check optional extensions
	missingExtensions.clear();
	for (const auto& extension : optionalExtensions.instanceExtensions)
	{
		if (availableExtensions.instanceExtensions.find(extension) != availableExtensions.instanceExtensions.end())
		{
			enabledExtensions.instanceExtensions.insert(extension);
		}
		else
		{
			missingExtensions.push_back(extension);
		}
	}

	if (!missingExtensions.empty())
	{
		LogWarning(LogVulkan) << "The following optional Vulkan instance extension(s) are not supported:" << endl;
		for (const auto& extension : missingExtensions)
		{
			LogWarning(LogVulkan) << "  - " << extension << endl;
		}
	}

	LogInfo(LogVulkan) << "Enabling Vulkan instance extensions:" << endl;
	for (const auto& extension : enabledExtensions.instanceExtensions)
	{
		LogInfo(LogVulkan) << "  - " << extension << endl;
	}

	m_instanceExtensionsValidated = true;

	return true;
}


bool ExtensionManager::ValidateDeviceExtensions()
{
	// Check required extensions
	vector<string> missingExtensions;
	for (const auto& extension : requiredExtensions.deviceExtensions)
	{
		if (availableExtensions.deviceExtensions.find(extension) != availableExtensions.deviceExtensions.end())
		{
			enabledExtensions.deviceExtensions.insert(extension);
		}
		else
		{
			missingExtensions.push_back(extension);
		}
	}

	if (!missingExtensions.empty())
	{
		LogError(LogVulkan) << "Failed to create Vulkan device because the following required extensions(s) are not supported:" << endl;
		for (const auto& extension : missingExtensions)
		{
			LogError(LogVulkan) << "  - " << extension << endl;
		}
		return false;
	}

	// Check optional extensions
	missingExtensions.clear();
	for (const auto& extension : optionalExtensions.deviceExtensions)
	{
		if (availableExtensions.deviceExtensions.find(extension) != availableExtensions.deviceExtensions.end())
		{
			enabledExtensions.deviceExtensions.insert(extension);
		}
		else
		{
			missingExtensions.push_back(extension);
		}
	}

	if (!missingExtensions.empty())
	{
		LogWarning(LogVulkan) << "The following optional Vulkan device extension(s) are not supported:" << endl;
		for (const auto& extension : missingExtensions)
		{
			LogWarning(LogVulkan) << "  - " << extension << endl;
		}
	}

	LogInfo(LogVulkan) << "Enabling Vulkan device extensions:" << endl;
	for (const auto& extension : enabledExtensions.deviceExtensions)
	{
		LogInfo(LogVulkan) << "  - " << extension << endl;
	}

	m_deviceExtensionsValidated = true;

	return true;
}

} // namespace Luna::VK