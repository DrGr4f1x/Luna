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

#include "Graphics\RayTracing\RayTracingGeometry.h"
#include "Graphics\Vulkan\VulkanCommon.h"


namespace Luna::VK
{

struct RayTracingBuildInfo : public RayTracingBuildInfoBase
{
	VkAccelerationStructureBuildGeometryInfoKHR buildGeometryInfo{};
	std::vector< VkAccelerationStructureBuildRangeInfoKHR*> rangeInfos{};
};

} // namespace Luna::VK
