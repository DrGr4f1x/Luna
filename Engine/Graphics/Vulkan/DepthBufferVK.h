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

#include "Graphics\DepthBuffer.h"
#include "Graphics\Vulkan\DescriptorVK.h"
#include "Graphics\Vulkan\VulkanCommon.h"

namespace Luna::VK
{

// Forward declarations
class Device;


class DepthBuffer : public IDepthBuffer
{
	friend class Device;

public:
	VkImage GetImage() const noexcept { return m_image->Get(); }
	const Descriptor& GetDescriptor(DepthStencilAspect depthStencilAspect) const noexcept;

protected:
	wil::com_ptr<CVkImage> m_image;
	Descriptor m_depthStencilDescriptor;
	Descriptor m_depthOnlyDescriptor;
	Descriptor m_stencilOnlyDescriptor;
};

} // namespace Luna::VK