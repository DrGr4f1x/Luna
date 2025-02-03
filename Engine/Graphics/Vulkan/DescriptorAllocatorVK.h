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

class DescriptorSetAllocator
{
public:
	VkDescriptorSet Allocate(VkDescriptorSetLayout layout);

	static void DestroyAll();

protected:
	static const uint32_t sm_numDescriptorsPerPool = 256;
	static std::mutex sm_allocationMutex;
	static std::vector<VkDescriptorPool> sm_descriptorPoolList;
	static VkDescriptorPool RequestNewPool();

	VkDescriptorType m_type;
	VkDescriptorPool m_descriptorPool{ VK_NULL_HANDLE };
};


// TODO: Refactor this.  Roll it entirely into GraphicsDevice?
extern DescriptorSetAllocator g_descriptorSetAllocator;
inline VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetLayout layout)
{
	return g_descriptorSetAllocator.Allocate(layout);
}

} // namespace Luna::VK