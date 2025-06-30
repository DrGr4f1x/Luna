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

#include "LinearAllocator12.h"

#include "Device12.h"
#include "DeviceManager12.h"


using namespace std;


namespace Luna::DX12
{

LinearAllocatorType LinearAllocatorPageManager::sm_autoType = kGpuExclusive;


LinearAllocatorPageManager::LinearAllocatorPageManager()
{
	m_allocationType = sm_autoType;
	sm_autoType = (LinearAllocatorType)(sm_autoType + 1);
	assert(sm_autoType <= kNumAllocatorTypes);
}


LinearAllocatorPageManager LinearAllocator::sm_pageManager[2];


LinearAllocationPage* LinearAllocatorPageManager::RequestPage()
{
	lock_guard<mutex> lockGuard(m_mutex);

	DeviceManager* deviceManager = GetD3D12DeviceManager();

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
		pagePtr = CreateNewPage();
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

	DeviceManager* deviceManager = GetD3D12DeviceManager();

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
	D3D12_HEAP_PROPERTIES heapProps;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resourceDesc;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment = 0;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ResourceState defaultUsage;

	if (m_allocationType == kGpuExclusive)
	{
		heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
		resourceDesc.Width = pageSize == 0 ? kGpuAllocatorPageSize : pageSize;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		defaultUsage = ResourceState::UnorderedAccess;
	}
	else
	{
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		resourceDesc.Width = pageSize == 0 ? kCpuAllocatorPageSize : pageSize;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		defaultUsage = ResourceState::GenericRead;
	}

	ID3D12Resource* buffer = nullptr;
	auto device = GetD3D12Device()->GetD3D12Device();

	assert_succeeded(device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&resourceDesc, ResourceStateToDX12(defaultUsage), nullptr, IID_PPV_ARGS(&buffer)));

	SetDebugName(buffer, "LinearAllocator Page");

	return new LinearAllocationPage(buffer, defaultUsage);
}


void LinearAllocator::CleanupUsedPages(uint64_t fenceID)
{
	sm_pageManager[m_allocationType].FreeLargePages(fenceID, m_largePageList);
	m_largePageList.clear();

	if (m_curPage == nullptr)
	{
		return;
	}

	m_retiredPages.push_back(m_curPage);
	m_curPage = nullptr;
	m_curOffset = 0;

	sm_pageManager[m_allocationType].DiscardPages(fenceID, m_retiredPages);
	m_retiredPages.clear();
}


DynAlloc LinearAllocator::AllocateLargePage(size_t sizeInBytes)
{
	LinearAllocationPage* oneOff = sm_pageManager[m_allocationType].CreateNewPage(sizeInBytes);
	m_largePageList.push_back(oneOff);

	DynAlloc ret(oneOff->GetResource(), 0, sizeInBytes);
	ret.dataPtr = oneOff->m_cpuVirtualAddress;
	ret.gpuAddress = oneOff->m_gpuVirtualAddress;

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
		m_curPage = sm_pageManager[m_allocationType].RequestPage();
		m_curOffset = 0;
	}

	DynAlloc ret(m_curPage->GetResource(), m_curOffset, alignedSize);
	ret.dataPtr = (uint8_t*)m_curPage->m_cpuVirtualAddress + m_curOffset;
	ret.gpuAddress = m_curPage->m_gpuVirtualAddress + m_curOffset;

	m_curOffset += alignedSize;

	return ret;
}

} // namespace Luna::DX12