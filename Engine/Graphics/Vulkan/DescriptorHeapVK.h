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

#include "Graphics\Vulkan\DescriptorAllocationVK.h"
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
	virtual DescriptorAllocation AllocateDescriptorMemory(VkDescriptorSetLayout vkDescriptorSetLayout) = 0;
	virtual VulkanDescriptorHeapClass GetClass() const = 0;

	virtual VkBuffer GetBuffer() const = 0;

	virtual VkBindHeapInfoEXT GetBindInfo() const = 0;
};


class DescriptorHeap : public IDescriptorHeap
{
	friend class Device;

public:
	virtual ~DescriptorHeap();
	DescriptorAllocation AllocateDescriptorMemory(VkDescriptorSetLayout vkDescriptorSetLayout);
	VulkanDescriptorHeapClass GetClass() const;

	VkBuffer GetBuffer() const;

private:
	IDescriptorHeapImpl* m_impl{ nullptr };
};


class DescriptorBufferImpl : public IDescriptorHeapImpl
{
	friend class Device;

public:
	DescriptorAllocation AllocateDescriptorMemory(VkDescriptorSetLayout vkDescriptorSetLayout) override;
	VulkanDescriptorHeapClass GetClass() const override { return VulkanDescriptorHeapClass::Buffer; }

	VkBuffer GetBuffer() const override { return m_descriptorBuffer->Get(); }

	VkBindHeapInfoEXT GetBindInfo() const override { return VkBindHeapInfoEXT{ .sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT }; }

protected:
	Device* m_device{ nullptr };
	size_t m_alignment{ 0 };
	size_t m_bufferSize{ 0 };
	size_t m_freeSpace{ 0 };
	std::byte* m_bufferHead{ nullptr };
	std::byte* m_initialHead{ nullptr };
	wil::com_ptr<CVkBuffer> m_descriptorBuffer;
};


class DescriptorHeapImpl : public IDescriptorHeapImpl
{
	friend class Device;

public:
	DescriptorAllocation AllocateDescriptorMemory(VkDescriptorSetLayout vkDescriptorSetLayout) override;
	VulkanDescriptorHeapClass GetClass() const override { return VulkanDescriptorHeapClass::Heap; }

	VkBuffer GetBuffer() const { return m_descriptorHeap->Get(); }

	VkBindHeapInfoEXT GetBindInfo() const override { return m_bindInfo; }

protected:
	Device* m_device{ nullptr };
	wil::com_ptr<CVkBuffer> m_descriptorHeap;
	VkBindHeapInfoEXT m_bindInfo{};
};

} // namespace Luna::VK