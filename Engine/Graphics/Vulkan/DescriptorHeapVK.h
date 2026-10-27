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

#include "Graphics\DescriptorHeap.h"

#include "Graphics\Vulkan\DescriptorAllocatorVK.h"
#include "Graphics\Vulkan\VulkanCommon.h"


namespace Luna::VK
{

// Forward declarations
class Device;


enum class VulkanDescriptorHeapClass
{
	Heap,
	Buffer
};


class IDescriptorHeapImpl
{
public:
	virtual DescriptorBufferAllocation AllocateDescriptorMemory(VkDescriptorSetLayout vkDescriptorSetLayout) = 0;
	virtual VulkanDescriptorHeapClass GetClass() const = 0;

	virtual VkBuffer GetBuffer() const = 0;
};


class DescriptorHeap : public IDescriptorHeap
{
	friend class Device;

public:
	virtual ~DescriptorHeap();
	DescriptorBufferAllocation AllocateDescriptorMemory(VkDescriptorSetLayout vkDescriptorSetLayout);
	VulkanDescriptorHeapClass GetClass() const;

	VkBuffer GetBuffer() const;

private:
	IDescriptorHeapImpl* m_impl{ nullptr };
};


class DescriptorBufferImpl : public IDescriptorHeapImpl
{
	friend class Device;

public:
	DescriptorBufferAllocation AllocateDescriptorMemory(VkDescriptorSetLayout vkDescriptorSetLayout) override;
	VulkanDescriptorHeapClass GetClass() const override { return VulkanDescriptorHeapClass::Buffer; }

	VkBuffer GetBuffer() const { return m_descriptorBuffer->Get(); }

protected:
	Device* m_device{ nullptr };
	size_t m_alignment{ 0 };
	size_t m_bufferSize{ 0 };
	size_t m_freeSpace{ 0 };
	std::byte* m_bufferHead{ nullptr };
	std::byte* m_initialHead{ nullptr };
	wil::com_ptr<CVkBuffer> m_descriptorBuffer;
};

} // namespace Luna::VK