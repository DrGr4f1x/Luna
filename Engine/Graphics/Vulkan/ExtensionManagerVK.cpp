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

bool ExtensionManager::InitializeInstance()
{
	bool bLayersRead = EnumerateInstanceLayers();
	bool bExtensionsRead = EnumerateInstanceExtensions();

	return bLayersRead && bExtensionsRead;
}


bool ExtensionManager::InitializeDevice(VkPhysicalDevice device)
{
	LogInfo(LogVulkan) << "Enumerating available Vulkan device extensions" << endl;

	uint32_t numExtensions{ 0 };
	if (VK_FAILED(vkEnumerateDeviceExtensionProperties(device, nullptr, &numExtensions, nullptr)))
	{
		LogError(LogVulkan) << "Failed to get Vulkan device extension count" << endl;
		return false;
	}

	vector<VkExtensionProperties> extensions(numExtensions);
	if (VK_FAILED(vkEnumerateDeviceExtensionProperties(device, nullptr, &numExtensions, extensions.data())))
	{
		LogError(LogVulkan) << "Failed to enumerate Vulkan device extensions" << endl;
		return false;
	}

	for (const auto& extension : extensions)
	{
		LogInfo(LogVulkan) << "  - " << extension.extensionName << endl;
		availableExtensions.deviceExtensions.insert(extension.extensionName);
	}

	return true;
}


bool ExtensionManager::GetEnabledInstanceLayers(vector<const char*>& layers)
{
	layers.clear();

	if (!m_instanceLayersValidated)
	{
		if (!ValidateInstanceLayers())
		{
			return false;
		}
	}

	for (const auto& layer : enabledExtensions.instanceLayers)
	{
		layers.push_back(layer.c_str());
	}

	return true;
}


bool ExtensionManager::GetEnabledInstanceExtensions(vector<const char*>& extensions)
{
	extensions.clear();

	if (!m_instanceExtensionsValidated)
	{
		if (!ValidateInstanceExtensions())
		{
			return false;
		}
	}

	for (const auto& extension : enabledExtensions.instanceExtensions)
	{
		extensions.push_back(extension.c_str());
	}

	return true;
}


bool ExtensionManager::GetEnabledDeviceExtensions(vector<const char*>& extensions)
{
	extensions.clear();

	if (!m_deviceExtensionsValidated)
	{
		if (!ValidateDeviceExtensions())
		{
			return false;
		}
	}

	for (const auto& extension : enabledExtensions.deviceExtensions)
	{
		extensions.push_back(extension.c_str());
	}

	return true;
}


bool ExtensionManager::EnumerateInstanceLayers()
{
	LogInfo(LogVulkan) << "Enumerating available Vulkan instance layers" << endl;

	uint32_t numLayers{ 0 };
	if (VK_FAILED(vkEnumerateInstanceLayerProperties(&numLayers, nullptr)))
	{
		LogError(LogVulkan) << "Failed to get Vulkan instance layer count" << endl;
		return false;
	}

	vector<VkLayerProperties> layers(numLayers);
	if (VK_FAILED(vkEnumerateInstanceLayerProperties(&numLayers, layers.data())))
	{
		LogError(LogVulkan) << "Failed to enumerate Vulkan instance layers" << endl;
		return false;
	}

	availableExtensions.instanceLayers.clear();
	for (const auto& layerProps : layers)
	{
		LogInfo(LogVulkan) << "  - " << layerProps.layerName << endl;
		availableExtensions.instanceLayers.insert(layerProps.layerName);
	}

	return true;
}


bool ExtensionManager::EnumerateInstanceExtensions()
{
	LogInfo(LogVulkan) << "Enumerating available Vulkan instance extensions" << endl;

	uint32_t numExtensions{ 0 };
	if (VK_FAILED(vkEnumerateInstanceExtensionProperties(nullptr, &numExtensions, nullptr)))
	{
		LogError(LogVulkan) << "Failed to get Vulkan instance extension count" << endl;
		return false;
	}

	vector<VkExtensionProperties> extensions(numExtensions);
	if (VK_FAILED(vkEnumerateInstanceExtensionProperties(nullptr, &numExtensions, extensions.data())))
	{
		LogError(LogVulkan) << "Failed to enumerate Vulkan instance extensions" << endl;
		return false;
	}

	availableExtensions.instanceExtensions.clear();
	for (const auto& extensionProps : extensions)
	{
		LogInfo(LogVulkan) << "  - " << extensionProps.extensionName << endl;
		availableExtensions.instanceExtensions.insert(extensionProps.extensionName);
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
		LogError(LogVulkan) << "Failed to create Vulkan instance because the following required layer(s) are not supported:";;
		for (const auto& layer :missingLayers)
		{
			LogError(LogVulkan) << endl << "  - " << layer;
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
		LogWarning(LogVulkan) << "The following optional Vulkan instance layer(s) are not supported:";;
		for (const auto& layer : missingLayers)
		{
			LogError(LogVulkan) << endl << "  - " << layer;
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
		LogError(LogVulkan) << "Failed to create Vulkan instance because the following required extensions(s) are not supported:";;
		for (const auto& extension : missingExtensions)
		{
			LogError(LogVulkan) << endl << "  - " << extension;
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
		LogWarning(LogVulkan) << "The following optional Vulkan instance extension(s) are not supported:";
		for (const auto& extension : missingExtensions)
		{
			LogError(LogVulkan) << endl << "  - " << extension;
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
		LogError(LogVulkan) << "Failed to create Vulkan device because the following required extensions(s) are not supported:";;
		for (const auto& extension : missingExtensions)
		{
			LogError(LogVulkan) << endl << "  - " << extension;
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
		LogWarning(LogVulkan) << "The following optional Vulkan device extension(s) are not supported:";
		for (const auto& extension : missingExtensions)
		{
			LogError(LogVulkan) << endl << "  - " << extension;
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