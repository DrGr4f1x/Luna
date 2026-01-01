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

#if USE_DESCRIPTOR_BUFFERS
struct DescriptorBufferAllocation
{
	std::byte* mem{ nullptr };
	size_t offset{ 0 };
};


class DescriptorBufferAllocator
{
	friend class Device;

public:
	DescriptorBufferAllocator(DescriptorBufferType bufferType, size_t sizeInBytes);
	DescriptorBufferAllocation Allocate(VkDescriptorSetLayout layout);

	static void CreateAll();
	static void DestroyAll();

	VkBuffer GetBuffer() const noexcept;

protected:
	static std::mutex sm_allocationMutex;

	Device* m_device{ nullptr };
	size_t m_alignment{ 0 };

	const DescriptorBufferType m_type{ DescriptorBufferType::Resource };
	const size_t m_bufferSize{ 0 };
	size_t m_freeSpace{ 0 };
	std::byte* m_bufferHead{ nullptr };
	std::byte* m_initialHead{ nullptr };
	wil::com_ptr<CVkBuffer> m_descriptorBuffer;

private:
	void Create();
};


extern DescriptorBufferAllocator g_userDescriptorBufferAllocator[];

inline DescriptorBufferAllocation AllocateDescriptorBufferMemory(DescriptorBufferType bufferType, VkDescriptorSetLayout layout)
{
	return g_userDescriptorBufferAllocator[(uint32_t)bufferType].Allocate(layout);
}
#endif // USE_DESCRIPTOR_BUFFERS


#if USE_LEGACY_DESCRIPTOR_SETS
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
#endif // USE_LEGACY_DESCRIPTOR_SETS

} // namespace Luna::VK