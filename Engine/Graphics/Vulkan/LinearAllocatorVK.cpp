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

#include "LinearAllocatorVK.h"

#include "DeviceVK.h"
#include "DeviceManagerVK.h"

using namespace std;


namespace Luna::VK
{

void LinearAllocationPage::Map()
{
	if (m_cpuVirtualAddress == nullptr)
	{
		ThrowIfFailed(vmaMapMemory(m_buffer->GetAllocator(), m_buffer->GetAllocation(), (void**)&m_cpuVirtualAddress));
	}
}


void LinearAllocationPage::Unmap()
{
	if (m_cpuVirtualAddress != nullptr)
	{
		vmaUnmapMemory(m_buffer->GetAllocator(), m_buffer->GetAllocation());
		m_cpuVirtualAddress = nullptr;
	}
}


LinearAllocatorPageManager::LinearAllocatorPageManager() = default;


LinearAllocatorPageManager LinearAllocator::sm_pageManager;


LinearAllocationPage* LinearAllocatorPageManager::RequestPage()
{
	lock_guard<mutex> lockGuard(m_mutex);

	auto deviceManager = GetVulkanDeviceManager();

	while (!m_retiredPages.empty() && deviceManager->IsFenceComplete(m_retiredPages.front().first))
	{
		m_availablePages.push(m_retiredPages.front().second);
		m_retiredPages.pop();
	}

	LinearAllocationPage* pagePtr = nullptr;

	if (!m_availablePages.empty())
	{
		pagePtr = m_availablePages.front();
		m_availablePages.pop();
	}
	else
	{
		pagePtr = CreateNewPage(kCpuAllocatorPageSize);
		m_pagePool.emplace_back(pagePtr);
	}

	return pagePtr;
}


void LinearAllocatorPageManager::DiscardPages(uint64_t fenceValue, const vector<LinearAllocationPage*>& usedPages)
{
	lock_guard<mutex> lockGuard(m_mutex);

	for (auto iter = usedPages.begin(); iter != usedPages.end(); ++iter)
	{
		m_retiredPages.push(make_pair(fenceValue, *iter));
	}
}


void LinearAllocatorPageManager::FreeLargePages(uint64_t fenceValue, const vector<LinearAllocationPage*>& largePages)
{
	lock_guard<mutex> lockGuard(m_mutex);

	auto deviceManager = GetVulkanDeviceManager();

	while (!m_deletionQueue.empty() && deviceManager->IsFenceComplete(m_deletionQueue.front().first))
	{
		delete m_deletionQueue.front().second;
		m_deletionQueue.pop();
	}

	for (auto iter = largePages.begin(); iter != largePages.end(); ++iter)
	{
		(*iter)->Unmap();
		m_deletionQueue.push(make_pair(fenceValue, *iter));
	}
}


LinearAllocationPage* LinearAllocatorPageManager::CreateNewPage(size_t pageSize)
{
	constexpr VkBufferUsageFlags transferFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	ResourceType type = ResourceType::ConstantBuffer | ResourceType::VertexBuffer | ResourceType::IndexBuffer;

	VkBufferCreateInfo bufferCreateInfo{
		.sType	= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size	= pageSize,
		.usage	= GetBufferUsageFlags(type) | transferFlags
	};

	VmaAllocationCreateInfo allocCreateInfo{};
	allocCreateInfo.flags = GetMemoryFlags(MemoryAccess::CpuMapped);
	allocCreateInfo.usage = GetMemoryUsage(MemoryAccess::CpuMapped);

	VkBuffer vkBuffer{ VK_NULL_HANDLE };
	VmaAllocation vmaBufferAllocation{ VK_NULL_HANDLE };

	auto deviceManager = GetVulkanDeviceManager();
	auto device = deviceManager->GetVulkanDevice();

	auto res = vmaCreateBuffer(deviceManager->GetAllocator()->Get(), &bufferCreateInfo, &allocCreateInfo, &vkBuffer, &vmaBufferAllocation, nullptr);
	assert(res == VK_SUCCESS);

	SetDebugName(device->Get(), vkBuffer, "Linear Allocator Page");

	auto buffer = Create<CVkBuffer>(device, deviceManager->GetAllocator(), vkBuffer, vmaBufferAllocation);

	LinearAllocationPage* page = new LinearAllocationPage(buffer.get(), ResourceState::GenericRead);
	return page;
}


void LinearAllocator::CleanupUsedPages(uint64_t fenceID)
{
	sm_pageManager.FreeLargePages(fenceID, m_largePageList);

	m_largePageList.clear();

	if (m_curPage == nullptr)
	{
		return;
	}

	m_retiredPages.push_back(m_curPage);
	m_curPage = nullptr;
	m_curOffset = 0;

	sm_pageManager.DiscardPages(fenceID, m_retiredPages);
	m_retiredPages.clear();
}


DynAlloc LinearAllocator::AllocateLargePage(size_t sizeInBytes)
{
	LinearAllocationPage* oneOff = sm_pageManager.CreateNewPage(sizeInBytes);
	m_largePageList.push_back(oneOff);

	DynAlloc ret{
		.resource	= oneOff->GetBuffer(),
		.offset		= 0,
		.size		= sizeInBytes,
		.dataPtr	= oneOff->m_cpuVirtualAddress
	};

	return ret;
}


DynAlloc LinearAllocator::Allocate(size_t sizeInBytes, size_t alignment)
{
	const size_t alignmentMask = alignment - 1;

	// Assert that it's a power of two.
	assert((alignmentMask & alignment) == 0);

	// Align the allocation
	const size_t alignedSize = Math::AlignUpWithMask(sizeInBytes, alignmentMask);

	if (alignedSize > m_pageSize)
	{
		return AllocateLargePage(alignedSize);
	}

	m_curOffset = Math::AlignUp(m_curOffset, alignment);

	if (m_curOffset + alignedSize > m_pageSize)
	{
		assert(m_curPage != nullptr);
		m_retiredPages.push_back(m_curPage);
		m_curPage = nullptr;
	}

	if (m_curPage == nullptr)
	{
		m_curPage = sm_pageManager.RequestPage();
		m_curOffset = 0;
	}

	DynAlloc ret{
		.resource	= m_curPage->GetBuffer(),
		.offset		= m_curOffset,
		.size		= alignedSize,
		.dataPtr	= (uint8_t*)m_curPage->m_cpuVirtualAddress + m_curOffset
	};

	m_curOffset += alignedSize;

	return ret;
}

} // namespace Luna::VK