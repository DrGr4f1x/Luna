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


class IDescriptorHeapVK : public IDescriptorHeap
{
public:
	virtual DescriptorAllocation AllocateDescriptorMemory(DescriptorHeapType heapType, VkDescriptorSetLayout vkDescriptorSetLayout) = 0;

	virtual DescriptorAllocation AllocateDescriptors(DescriptorClass descriptorClass, uint32_t numDescriptors) = 0;

	virtual DescriptorManagement GetDescriptorManagement() const = 0;

	virtual VkBuffer GetBuffer(DescriptorHeapType heapType) const = 0;

	virtual VkBindHeapInfoEXT GetBindInfo(DescriptorHeapType heapType) const = 0;

	virtual uint32_t GetNumImageDescriptors() const = 0;
	virtual uint32_t GetNumBufferDescriptors() const = 0;
	virtual uint32_t GetNumSamplerDescriptors() const = 0;
	virtual uint32_t GetNumResourceDescriptors() const = 0;
};


class DescriptorHeap : public IDescriptorHeapVK
{
	friend class Device;

public:
	~DescriptorHeap() override;

	DescriptorAllocation AllocateDescriptorMemory(DescriptorHeapType heapType, VkDescriptorSetLayout vkDescriptorSetLayout) override;

	DescriptorAllocation AllocateDescriptors(DescriptorClass descriptorClass, uint32_t numDescriptors) override;

	DescriptorManagement GetDescriptorManagement() const override { return DescriptorManagement::Heap; }

	VkBuffer GetBuffer(DescriptorHeapType heapType) const override;

	VkBindHeapInfoEXT GetBindInfo(DescriptorHeapType heapType) const override;

	uint32_t GetNumImageDescriptors() const override { return m_numImageDescriptors; }
	uint32_t GetNumBufferDescriptors() const override { return m_numBufferDescriptors; }
	uint32_t GetNumSamplerDescriptors() const override { return m_numSamplerDescriptors; }
	uint32_t GetNumResourceDescriptors() const override { return m_numImageDescriptors + m_numBufferDescriptors; }

private:
	Device* m_device{ nullptr };

	struct HeapState
	{
		wil::com_ptr<CVkBuffer> descriptorHeap;
		VkBindHeapInfoEXT bindInfo{};
		std::byte* bufferHead{ nullptr };
	};

	HeapState m_resourceState{};
	HeapState m_samplerState{};

	// Initial capacity
	uint32_t m_numImageDescriptors{ 0 };
	uint32_t m_numBufferDescriptors{ 0 };
	uint32_t m_numSamplerDescriptors{ 0 };

	// Current allocation
	uint32_t m_numAllocatedImageDescriptors{ 0 };
	uint32_t m_numAllocatedBufferDescriptors{ 0 };
	uint32_t m_numAllocatedSamplerDescriptors{ 0 };

	// Offset to each sub-buffer
	uint32_t m_imageDescriptorOffset{ 0 };
	uint32_t m_bufferDescriptorOffset{ 0 };
	uint32_t m_samplerDescriptorOffset{ 0 };

private:
	size_t GetImageDescriptorSize() const;
	size_t GetBufferDescriptorSize() const;
	size_t GetSamplerDescriptorSize() const;
};


class DescriptorBuffer : public IDescriptorHeapVK
{
	friend class Device;

public:
	~DescriptorBuffer() override;

	DescriptorAllocation AllocateDescriptorMemory(DescriptorHeapType heapType, VkDescriptorSetLayout vkDescriptorSetLayout) override;

	DescriptorAllocation AllocateDescriptors(DescriptorClass descriptorClass, uint32_t numDescriptors) override;

	DescriptorManagement GetDescriptorManagement() const override { return DescriptorManagement::Buffer; }

	VkBuffer GetBuffer(DescriptorHeapType heapType) const override;

	VkBindHeapInfoEXT GetBindInfo(DescriptorHeapType heapType) const override { assert(false);  return VkBindHeapInfoEXT{}; }

	uint32_t GetNumImageDescriptors() const { assert(false);  return 0; }
	uint32_t GetNumBufferDescriptors() const { assert(false);  return 0; }
	uint32_t GetNumSamplerDescriptors() const { return m_numSamplerDescriptors; }
	uint32_t GetNumResourceDescriptors() const { return m_numResourceDescriptors; }

private:
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

	uint32_t m_numResourceDescriptors{ 0 };
	uint32_t m_numSamplerDescriptors{ 0 };

	BufferState m_resourceBuffer{};
	BufferState m_samplerBuffer{};
};

} // namespace Luna::VK