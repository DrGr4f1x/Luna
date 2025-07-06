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

struct Semaphore
{
	std::string name{ "Unnamed Semaphore" };
	wil::com_ptr<CVkSemaphore> semaphore;
	bool isBinary{ true };
	uint64_t value{ 0 };
};

using SemaphorePtr = std::shared_ptr<Semaphore>;

} // namespace Luna::VK