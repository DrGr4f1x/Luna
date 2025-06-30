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

#include "Graphics\DX12\DirectXCommon.h"

#define DEFAULT_ALIGN 256

// TODO: Replace this entire thing with D3D12MA (See D3D12Sample.cpp in the D3D12MemoryAllocator project)

namespace Luna::DX12
{

struct DynAlloc
{
	DynAlloc(ID3D12Resource* baseResource, size_t thisOffset, size_t thisSize)
		: buffer{ baseResource }
		, offset{ thisOffset }
		, size{ thisSize }
	{}

	ID3D12Resource* buffer{ nullptr };
	size_t offset{ 0 };
	size_t size{ 0 };
	void* dataPtr{ nullptr };
	D3D12_GPU_VIRTUAL_ADDRESS gpuAddress{ D3D12_GPU_VIRTUAL_ADDRESS_NULL };
};


class LinearAllocationPage
{
public:
	LinearAllocationPage(ID3D12Resource* resource, ResourceState usage)
	{
		m_resource.attach(resource);
		m_usageState = usage;
		m_gpuVirtualAddress = m_resource->GetGPUVirtualAddress();
		m_resource->Map(0, nullptr, &m_cpuVirtualAddress);
	}

	
	~LinearAllocationPage()
	{
		Unmap();
	}


	void Map()
	{
		if (m_cpuVirtualAddress == nullptr)
		{
			m_resource->Map(0, nullptr, &m_cpuVirtualAddress);
		}
	}


	void Unmap()
	{
		if (m_cpuVirtualAddress != nullptr)
		{
			m_resource->Unmap(0, nullptr);
			m_cpuVirtualAddress = nullptr;
		}
	}


	ID3D12Resource* GetResource() const { return m_resource.get(); }

public:
	void* m_cpuVirtualAddress{ nullptr };
	D3D12_GPU_VIRTUAL_ADDRESS m_gpuVirtualAddress{ D3D12_GPU_VIRTUAL_ADDRESS_NULL };


protected:
	wil::com_ptr<ID3D12Resource> m_resource;
	ResourceState m_usageState{ ResourceState::Undefined };
	ResourceState m_transitioningState{ ResourceState::Undefined };
	ResourceType m_type{ ResourceType::Unknown };
};


enum LinearAllocatorType
{
	kInvalidAllocator = -1,

	kGpuExclusive = 0,		// DEFAULT   GPU-writeable (via UAV)
	kCpuWritable = 1,		// UPLOAD CPU-writeable (but write combined)

	kNumAllocatorTypes
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
	static LinearAllocatorType sm_autoType;

	LinearAllocatorType m_allocationType;
	std::vector<std::unique_ptr<LinearAllocationPage>> m_pagePool;
	std::queue<std::pair<uint64_t, LinearAllocationPage*>> m_retiredPages;
	std::queue<std::pair<uint64_t, LinearAllocationPage*>> m_deletionQueue;
	std::queue<LinearAllocationPage*> m_availablePages;
	std::mutex m_mutex;
};


class LinearAllocator
{
public:

	LinearAllocator(LinearAllocatorType type)
		: m_allocationType(type)
		, m_pageSize(0)
		, m_curOffset(~(size_t)0)
		, m_curPage(nullptr)
	{
		assert(type > kInvalidAllocator && type < kNumAllocatorTypes);
		m_pageSize = (type == kGpuExclusive ? kGpuAllocatorPageSize : kCpuAllocatorPageSize);
	}


	DynAlloc Allocate(size_t sizeInBytes, size_t alignment = DEFAULT_ALIGN);

	void CleanupUsedPages(uint64_t fenceID);


	static void DestroyAll()
	{
		sm_pageManager[0].Destroy();
		sm_pageManager[1].Destroy();
	}

private:
	DynAlloc AllocateLargePage(size_t sizeInBytes);

	static LinearAllocatorPageManager sm_pageManager[2];

	LinearAllocatorType m_allocationType;
	size_t m_pageSize;
	size_t m_curOffset;
	LinearAllocationPage* m_curPage{ nullptr };
	std::vector<LinearAllocationPage*> m_retiredPages;
	std::vector<LinearAllocationPage*> m_largePageList;
};

} // namespace Luna::DX12