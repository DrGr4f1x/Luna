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
#include "Graphics\Vulkan\RefCountingVK.h"

using namespace Microsoft::WRL;


namespace Luna::VK
{

//
// VkInstance
//
class __declspec(uuid("D7BB5E27-8C83-4AF3-B5F6-92CEF5575057")) CVkInstance
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, IVkInstance>
	, public NonCopyable
{
public:
	CVkInstance() noexcept = default;
	explicit CVkInstance(VkInstance instance) noexcept
		: m_instance{ instance }
	{
	}

	~CVkInstance() final
	{
		Destroy();
	}

	VkInstance Get() const noexcept final { return m_instance; }
	operator VkInstance() const noexcept { return Get(); }

	void Destroy();

private:
	VkInstance m_instance{ VK_NULL_HANDLE };
};


//
// VkPhysicalDevice
//
class __declspec(uuid("9DF46137-E220-42FC-BE3A-9A3695F58F83")) CVkPhysicalDevice
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, IVkPhysicalDevice>
	, public NonCopyable
{
public:
	CVkPhysicalDevice() noexcept = default;
	CVkPhysicalDevice(IVkInstance* cinstance, VkPhysicalDevice physicalDevice) noexcept
		: m_instance{ cinstance }
		, m_physicalDevice{ physicalDevice }
	{
	}

	~CVkPhysicalDevice() final
	{
		Destroy();
	}

	VkPhysicalDevice Get() const noexcept { return m_physicalDevice; }
	operator VkPhysicalDevice() const noexcept { return Get(); }

	VkInstance GetInstance() const noexcept { return *m_instance; }

	void Destroy();

private:
	wil::com_ptr<IVkInstance> m_instance;
	VkPhysicalDevice m_physicalDevice{ VK_NULL_HANDLE };
};


//
// VkDevice
//
class __declspec(uuid("883F7812-D71F-42E1-B6D1-A9FBB842FE78")) CVkDevice
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, IVkDevice>
	, public NonCopyable
{
public:
	CVkDevice() noexcept = default;
	CVkDevice(IVkPhysicalDevice* physicalDevice, VkDevice device) noexcept
		: m_physicalDevice{ physicalDevice }
		, m_device{ device }
	{
	}

	~CVkDevice() final
	{
		Destroy();
	}

	VkDevice Get() const noexcept { return m_device; }
	operator VkDevice() const { return Get(); }

	VkPhysicalDevice GetPhysicalDevice() const noexcept { return *m_physicalDevice; }

	void Destroy();

private:
	wil::com_ptr<IVkPhysicalDevice> m_physicalDevice;
	VkDevice m_device{ VK_NULL_HANDLE };
};


//
// VkSurfaceKHR
//
class __declspec(uuid("B61E2455-F5E2-40D1-A4D7-06A7ACCAC9D9")) CVkSurface 
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, IVkSurfaceKHR>
	, public NonCopyable
{
public:
	CVkSurface() noexcept = default;
	CVkSurface(IVkInstance* instance, VkSurfaceKHR surface) noexcept
		: m_instance{ instance }
		, m_surfaceKHR{ surface }
	{
	}

	~CVkSurface() final
	{
		Destroy();
	}

	VkSurfaceKHR Get() const noexcept { return m_surfaceKHR; }
	operator VkSurfaceKHR() const noexcept { return Get(); }

	VkInstance GetInstance() const noexcept { return *m_instance; }

	void Destroy();

private:
	wil::com_ptr<IVkInstance> m_instance;
	VkSurfaceKHR m_surfaceKHR{ VK_NULL_HANDLE };
};

} // namespace Luna::VK