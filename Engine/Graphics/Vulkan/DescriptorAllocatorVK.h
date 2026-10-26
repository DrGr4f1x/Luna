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

struct DescriptorBufferAllocation
{
	std::byte* mem{ nullptr };
	size_t offset{ 0 };
};


class DescriptorBufferAllocator
{
	friend class Device;

public:
	DescriptorBufferAllocator(DescriptorBufferType bufferType);
	DescriptorBufferAllocation Allocate(VkDescriptorSetLayout layout);

	static void CreateAll();
	static void DestroyAll();

	VkBuffer GetBuffer() const noexcept;

	static size_t GetBufferSize(DescriptorBufferType bufferType);

protected:
	static std::mutex sm_allocationMutex;
	static const size_t sm_numResourceDescriptors = (1 << 16);
	static const size_t sm_numSamplerDescriptors = (1 << 10);

	Device* m_device{ nullptr };
	size_t m_alignment{ 0 };

	const DescriptorBufferType m_type{ DescriptorBufferType::Resource };
	size_t m_bufferSize{ 0 };
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

} // namespace Luna::VK