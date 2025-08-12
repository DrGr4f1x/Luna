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

#include "Graphics\Texture.h"
#include "Graphics\Vulkan\DescriptorVK.h"
#include "Graphics\Vulkan\VulkanCommon.h"


namespace Luna::VK
{

// Forward declarations
class Device;


class Texture : public ITexture
{
	friend class Device;

public:
	bool IsValid() const override { return m_image != nullptr; }

	const IDescriptor* GetDescriptor() const override { return &m_descriptor; }

	VkImage GetImage() const { return m_image->Get(); }
	const Descriptor& GetSrvDescriptor() const { return m_descriptor; }

protected:
	Device* m_device{ nullptr };
	wil::com_ptr<CVkImage> m_image;
	Descriptor m_descriptor;
	std::string m_name;
};

} // namespace Luna::VK