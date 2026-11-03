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


namespace Luna::VK
{

class DescriptorHeapBindingTemplate
{
	friend class Device;

public:
	uint32_t GetBindingInfo(uint32_t bindingIndex) const noexcept;

private:
	std::unordered_map<uint32_t, uint32_t> m_bindingInfoMap;
};

} // namespace Luna::VK