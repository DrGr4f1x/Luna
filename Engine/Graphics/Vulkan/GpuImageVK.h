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

#include "Graphics\PlatformData.h"
#include "Graphics\Vulkan\VulkanCommon.h"


namespace Luna::VK
{

class __declspec(uuid("2C896D53-646F-4D3D-9CE9-24CB12EF5CC7")) IGpuImageData : public IPlatformData
{
public:
	virtual VkImage GetImage() const noexcept = 0;
};

} // namespace Luna::VK