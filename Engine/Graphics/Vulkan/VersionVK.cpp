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

#include "VersionVK.h"

#include "VulkanCommon.h"

using namespace std;


namespace Luna::VK
{

VulkanVersionInfo DecodeVulkanVersion(uint32_t packedVersion)
{
	VulkanVersionInfo info{};
	info.variant = VK_API_VERSION_VARIANT(packedVersion);
	info.major = VK_API_VERSION_MAJOR(packedVersion);
	info.minor = VK_API_VERSION_MINOR(packedVersion);
	info.patch = VK_API_VERSION_PATCH(packedVersion);
	return info;
}


string VulkanVersionInfoToString(VulkanVersionInfo versionInfo)
{
	return format("{}.{}.{}", versionInfo.major, versionInfo.minor, versionInfo.patch);
}


string VulkanVersionToString(uint32_t packedVersion)
{
	return VulkanVersionInfoToString(DecodeVulkanVersion(packedVersion));
}

} // namespace Luna::VK