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
	virtual DescriptorAllocation AllocateDescriptorMemory(DescriptorHeapType heapType, VkDescriptorSetLayout vkDescriptorSetLayout) = 0;
	virtual VulkanDescriptorHeapClass GetClass() const = 0;

	virtual VkBuffer GetBuffer(DescriptorHeapType heapType) const = 0;

	virtual VkBindHeapInfoEXT GetBindInfo(DescriptorHeapType heapType) const = 0;
};


class DescriptorHeap : public IDescriptorHeap
{
	friend class Device;

public:
	virtual ~DescriptorHeap();
	DescriptorAllocation AllocateDescriptorMemory(DescriptorHeapType heapType, VkDescriptorSetLayout vkDescriptorSetLayout);
	VulkanDescriptorHeapClass GetClass() const;

	VkBuffer GetBuffer(DescriptorHeapType heapType) const;

private:
	IDescriptorHeapImpl* m_impl{ nullptr };
};


class DescriptorBufferImpl : public IDescriptorHeapImpl
{
	friend class Device;

public:
	DescriptorAllocation AllocateDescriptorMemory(DescriptorHeapType heapType, VkDescriptorSetLayout vkDescriptorSetLayout) override;
	VulkanDescriptorHeapClass GetClass() const override { return VulkanDescriptorHeapClass::Buffer; }

	VkBuffer GetBuffer(DescriptorHeapType heapType) const override;

	VkBindHeapInfoEXT GetBindInfo(DescriptorHeapType heapType) const override { return VkBindHeapInfoEXT{ .sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT }; }

protected:
	Device* m_device{ nullptr };

	struct BufferState
	{
		size_t alignment{ 0 };
		size_t bufferSize{ 0 };
		size_t freeSpace{ 0 };
		std::byte* bufferHead{ nullptr };
		std::byte* initialHead{ nullptr };
		wil::com_ptr<CVkBuffer> descriptorBuffer;
	};

	BufferState m_resourceBuffer;
	BufferState m_samplerBuffer;
};


class DescriptorHeapImpl : public IDescriptorHeapImpl
{
	friend class Device;

public:
	DescriptorAllocation AllocateDescriptorMemory(DescriptorHeapType heapType, VkDescriptorSetLayout vkDescriptorSetLayout) override;
	VulkanDescriptorHeapClass GetClass() const override { return VulkanDescriptorHeapClass::Heap; }

	VkBuffer GetBuffer(DescriptorHeapType heapType) const override;

	VkBindHeapInfoEXT GetBindInfo(DescriptorHeapType heapType) const override;

protected:
	Device* m_device{ nullptr };
	wil::com_ptr<CVkBuffer> m_descriptorHeap;
	VkBindHeapInfoEXT m_bindInfo{};
};

} // namespace Luna::VK