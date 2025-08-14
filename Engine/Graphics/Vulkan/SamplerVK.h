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

#include "Graphics\Sampler.h"
#include "Graphics\Vulkan\DescriptorVK.h"
#include "Graphics\Vulkan\VulkanCommon.h"

namespace Luna::VK
{

// Forward declarations
class Device;


class Sampler : public ISampler
{
	friend class Device;

public:
	const IDescriptor* GetDescriptor() const noexcept override { return &m_descriptor; }

	VkSampler GetSampler() const { return m_descriptor.GetSampler(); }

protected:
	Descriptor m_descriptor;
};

} // namespace Luna::VK