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

// Constant blocks must be multiples of 16 constants @ 16 bytes each
#define DEFAULT_ALIGN 256


namespace Luna::VK
{

class LinearAllocationPage
{
public:
	LinearAllocationPage(CVkBuffer* buffer, ResourceState usage)
		: m_buffer{ buffer }
		, m_usage{ usage }
	{
		Map();
	}

	~LinearAllocationPage()
	{
		Unmap();
	}

	void Map();
	void Unmap();

	VkBuffer GetBuffer() const { return m_buffer->Get(); }

	void* m_cpuVirtualAddress{ nullptr };

protected:
	wil::com_ptr<CVkBuffer> m_buffer;
	ResourceType m_type{ ResourceType::ConstantBuffer | ResourceType::VertexBuffer | ResourceType::IndexBuffer };
	ResourceState m_usage{ ResourceState::Undefined };
};


enum
{
	kGpuAllocatorPageSize = 0x10000,	// 64K
	kCpuAllocatorPageSize = 0x200000	// 2MB
};


class LinearAllocatorPageManager
{
public:

	LinearAllocatorPageManager();
	LinearAllocationPage* RequestPage();
	LinearAllocationPage* CreateNewPage(size_t pageSize = 0);

	// Discarded pages will get recycled.  This is for fixed size pages.
	void DiscardPages(uint64_t fenceID, const std::vector<LinearAllocationPage*>& pages);

	// Freed pages will be destroyed once their fence has passed.  This is for single-use,
	// "large" pages.
	void FreeLargePages(uint64_t fenceID, const std::vector<LinearAllocationPage*>& pages);

	void Destroy() { m_pagePool.clear(); }

private:
	std::vector<std::unique_ptr<LinearAllocationPage>> m_pagePool;
	std::queue<std::pair<uint64_t, LinearAllocationPage*>> m_retiredPages;
	std::queue<std::pair<uint64_t, LinearAllocationPage*>> m_deletionQueue;
	std::queue<LinearAllocationPage*> m_availablePages;
	std::mutex m_mutex;
};


class LinearAllocator
{
public:

	LinearAllocator()
		: m_pageSize(0)
		, m_curOffset(~(size_t)0)
		, m_curPage(nullptr)
	{
		m_pageSize = kCpuAllocatorPageSize;
	}


	DynAlloc Allocate(size_t sizeInBytes, size_t alignment = DEFAULT_ALIGN);

	void CleanupUsedPages(uint64_t fenceID);


	static void DestroyAll()
	{
		sm_pageManager.Destroy();
	}

private:
	DynAlloc AllocateLargePage(size_t sizeInBytes);

	static LinearAllocatorPageManager sm_pageManager;

	size_t m_pageSize;
	size_t m_curOffset;
	LinearAllocationPage* m_curPage{ nullptr };
	std::vector<LinearAllocationPage*> m_retiredPages;
	std::vector<LinearAllocationPage*> m_largePageList;
};

} // namespace Luna::VK