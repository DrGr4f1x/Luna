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

#include "DynamicDescriptorHeap12.h"

#include "CommandContext12.h"
#include "Device12.h"
#include "DeviceManager12.h"
#include "RootSignature12.h"

using namespace std;


namespace Luna::DX12
{

// Static members
mutex DynamicDescriptorHeap::sm_mutex;
vector<wil::com_ptr<ID3D12DescriptorHeap>> DynamicDescriptorHeap::sm_descriptorHeapPool[2];
queue<pair<uint64_t, ID3D12DescriptorHeap*>> DynamicDescriptorHeap::sm_retiredDescriptorHeaps[2];
queue<ID3D12DescriptorHeap*> DynamicDescriptorHeap::sm_availableDescriptorHeaps[2];


DynamicDescriptorHeap::DynamicDescriptorHeap(CommandContext12& owningContext, D3D12_DESCRIPTOR_HEAP_TYPE heapType)
	: m_owningContext(owningContext)
	, m_descriptorType(heapType)
{
	m_currentHeapPtr = nullptr;
	m_currentOffset = 0;
	m_descriptorSize = GetDescriptorHandleIncrementSize(heapType);
}


void DynamicDescriptorHeap::CleanupUsedHeaps(uint64_t fenceValue)
{
	RetireCurrentHeap();
	RetireUsedHeaps(fenceValue);
	m_graphicsHandleCache.ClearCache();
	m_computeHandleCache.ClearCache();
}


D3D12_GPU_DESCRIPTOR_HANDLE DynamicDescriptorHeap::UploadDirect(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	if (!HasSpace(1))
	{
		RetireCurrentHeap();
		UnbindAllValid();
	}

	m_owningContext.SetDescriptorHeap(m_descriptorType, GetHeapPointer());

	DescriptorHandle destHandle = m_firstDescriptor + m_currentOffset * m_descriptorSize;
	m_currentOffset += 1;

	auto device = GetD3D12Device();

	device->GetD3D12Device()->CopyDescriptorsSimple(1, destHandle.GetCpuHandle(), handle, m_descriptorType);

	return destHandle.GetGpuHandle();
}


/* static */ ID3D12DescriptorHeap* DynamicDescriptorHeap::RequestDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType)
{
	lock_guard<mutex> CS(sm_mutex);

	uint32_t idx = heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? 1 : 0;

	while (!sm_retiredDescriptorHeaps[idx].empty() && GetD3D12DeviceManager()->IsFenceComplete(sm_retiredDescriptorHeaps[idx].front().first))
	{
		sm_availableDescriptorHeaps[idx].push(sm_retiredDescriptorHeaps[idx].front().second);
		sm_retiredDescriptorHeaps[idx].pop();
	}

	if (!sm_availableDescriptorHeaps[idx].empty())
	{
		ID3D12DescriptorHeap* heapPtr = sm_availableDescriptorHeaps[idx].front();
		sm_availableDescriptorHeaps[idx].pop();
		return heapPtr;
	}
	else
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.Type = heapType;
		heapDesc.NumDescriptors = NumDescriptorsPerHeap;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.NodeMask = 1;
		wil::com_ptr<ID3D12DescriptorHeap> heapPtr;
		auto device = GetD3D12Device();
		assert_succeeded(device->GetD3D12Device()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heapPtr)));
		sm_descriptorHeapPool[idx].emplace_back(heapPtr);
		return heapPtr.get();
	}
}


/* static */ void DynamicDescriptorHeap::DiscardDescriptorHeaps(D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint64_t fenceValue, const vector<ID3D12DescriptorHeap*>& usedHeaps)
{
	const uint32_t idx = heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? 1 : 0;

	lock_guard<mutex> LockGuard(sm_mutex);

	for (auto iter = usedHeaps.begin(); iter != usedHeaps.end(); ++iter)
	{
		sm_retiredDescriptorHeaps[idx].push(make_pair(fenceValue, *iter));
	}
}


void DynamicDescriptorHeap::RetireCurrentHeap()
{
	// Don't retire unused heaps.
	if (m_currentOffset == 0)
	{
		assert(m_currentHeapPtr == nullptr);
		return;
	}

	assert(m_currentHeapPtr != nullptr);
	m_retiredHeaps.push_back(m_currentHeapPtr);
	m_currentHeapPtr = nullptr;
	m_currentOffset = 0;
}


void DynamicDescriptorHeap::RetireUsedHeaps(uint64_t fenceValue)
{
	DiscardDescriptorHeaps(m_descriptorType, fenceValue, m_retiredHeaps);
	m_retiredHeaps.clear();
}


inline ID3D12DescriptorHeap* DynamicDescriptorHeap::GetHeapPointer()
{
	if (m_currentHeapPtr == nullptr)
	{
		assert(m_currentOffset == 0);
		m_currentHeapPtr = RequestDescriptorHeap(m_descriptorType);
		m_firstDescriptor = DescriptorHandle(
			m_currentHeapPtr->GetCPUDescriptorHandleForHeapStart(),
			m_currentHeapPtr->GetGPUDescriptorHandleForHeapStart());
	}

	return m_currentHeapPtr;
}


void DynamicDescriptorHeap::CopyAndBindStagedTables(DynamicDescriptorHeap::DescriptorHandleCache& handleCache, ID3D12GraphicsCommandList* cmdList,
	void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE))
{
	uint32_t neededSize = handleCache.ComputeStagedSize();
	if (!HasSpace(neededSize))
	{
		RetireCurrentHeap();
		UnbindAllValid();
		neededSize = handleCache.ComputeStagedSize();
	}

	// This can trigger the creation of a new heap
	m_owningContext.SetDescriptorHeap(m_descriptorType, GetHeapPointer());
	handleCache.CopyAndBindStaleTables(m_descriptorType, m_descriptorSize, Allocate(neededSize), cmdList, SetFunc);
}


void DynamicDescriptorHeap::UnbindAllValid()
{
	m_graphicsHandleCache.UnbindAllValid();
	m_computeHandleCache.UnbindAllValid();
}


uint32_t DynamicDescriptorHeap::DescriptorHandleCache::ComputeStagedSize()
{
	// Sum the maximum assigned offsets of stale descriptor tables to determine total needed space.
	uint32_t neededSpace = 0;
	uint32_t rootIndex;
	uint32_t staleParams = m_staleRootParamsBitMap;
	while (_BitScanForward((unsigned long*)&rootIndex, staleParams))
	{
		staleParams ^= (1 << rootIndex);

		uint32_t maxSetHandle;
		assert_msg(TRUE == _BitScanReverse((unsigned long*)&maxSetHandle, m_rootDescriptorTable[rootIndex].assignedHandlesBitMap),
			"Root entry marked as stale but has no stale descriptors");

		neededSpace += maxSetHandle + 1;
	}
	return neededSpace;
}


void DynamicDescriptorHeap::DescriptorHandleCache::CopyAndBindStaleTables(
	D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t descriptorSize,
	DescriptorHandle destHandleStart, ID3D12GraphicsCommandList* cmdList,
	void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE))
{
	uint32_t staleParamCount = 0;
	uint32_t tableSize[DescriptorHandleCache::kMaxNumDescriptorTables];
	uint32_t rootIndices[DescriptorHandleCache::kMaxNumDescriptorTables];
	uint32_t neededSpace = 0;
	uint32_t rootIndex;

	// Sum the maximum assigned offsets of stale descriptor tables to determine total needed space.
	uint32_t staleParams = m_staleRootParamsBitMap;
	while (_BitScanForward((unsigned long*)&rootIndex, staleParams))
	{
		rootIndices[staleParamCount] = rootIndex;
		staleParams ^= (1 << rootIndex);

		uint32_t maxSetHandle;
		assert_msg(TRUE == _BitScanReverse((unsigned long*)&maxSetHandle, m_rootDescriptorTable[rootIndex].assignedHandlesBitMap),
			"Root entry marked as stale but has no stale descriptors");

		neededSpace += maxSetHandle + 1;
		tableSize[staleParamCount] = maxSetHandle + 1;

		++staleParamCount;
	}

	assert_msg(staleParamCount <= DescriptorHandleCache::kMaxNumDescriptorTables,
		"We're only equipped to handle so many descriptor tables");

	m_staleRootParamsBitMap = 0;

	static const uint32_t kMaxDescriptorsPerCopy = 16;
	uint32_t numDestDescriptorRanges = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE destDescriptorRangeStarts[kMaxDescriptorsPerCopy];
	uint32_t destDescriptorRangeSizes[kMaxDescriptorsPerCopy];

	uint32_t numSrcDescriptorRanges = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptorRangeStarts[kMaxDescriptorsPerCopy];
	uint32_t srcDescriptorRangeSizes[kMaxDescriptorsPerCopy];

	auto device = GetD3D12Device();

	for (uint32_t i = 0; i < staleParamCount; ++i)
	{
		rootIndex = rootIndices[i];
		(cmdList->*SetFunc)(rootIndex, destHandleStart.GetGpuHandle());

		DescriptorTableCache& rootDescTable = m_rootDescriptorTable[rootIndex];

		D3D12_CPU_DESCRIPTOR_HANDLE* srcHandles = rootDescTable.tableStart;
		uint64_t setHandles = (uint64_t)rootDescTable.assignedHandlesBitMap;
		D3D12_CPU_DESCRIPTOR_HANDLE curDest = destHandleStart.GetCpuHandle();
		destHandleStart += tableSize[i] * descriptorSize;

		unsigned long skipCount;
		while (_BitScanForward64(&skipCount, setHandles))
		{
			// Skip over unset descriptor handles
			setHandles >>= skipCount;
			srcHandles += skipCount;
			curDest.ptr += skipCount * descriptorSize;

			unsigned long descriptorCount;
			_BitScanForward64(&descriptorCount, ~setHandles);
			setHandles >>= descriptorCount;

			// If we run out of temp room, copy what we've got so far
			if (numSrcDescriptorRanges + descriptorCount > kMaxDescriptorsPerCopy)
			{
				device->GetD3D12Device()->CopyDescriptors(
					numDestDescriptorRanges, destDescriptorRangeStarts, destDescriptorRangeSizes,
					numSrcDescriptorRanges, srcDescriptorRangeStarts, srcDescriptorRangeSizes,
					type);

				numSrcDescriptorRanges = 0;
				numDestDescriptorRanges = 0;
			}

			// Setup destination range
			destDescriptorRangeStarts[numDestDescriptorRanges] = curDest;
			destDescriptorRangeSizes[numDestDescriptorRanges] = descriptorCount;
			++numDestDescriptorRanges;

			// Setup source ranges (one descriptor each because we don't assume they are contiguous)
			for (uint32_t j = 0; j < descriptorCount; ++j)
			{
				srcDescriptorRangeStarts[numSrcDescriptorRanges] = srcHandles[j];
				srcDescriptorRangeSizes[numSrcDescriptorRanges] = 1;
				++numSrcDescriptorRanges;
			}

			// Move the destination pointer forward by the number of descriptors we will copy
			srcHandles += descriptorCount;
			curDest.ptr += descriptorCount * descriptorSize;
		}
	}

	device->GetD3D12Device()->CopyDescriptors(
		numDestDescriptorRanges, destDescriptorRangeStarts, destDescriptorRangeSizes,
		numSrcDescriptorRanges, srcDescriptorRangeStarts, srcDescriptorRangeSizes,
		type);
}


void DynamicDescriptorHeap::DescriptorHandleCache::UnbindAllValid()
{
	m_staleRootParamsBitMap = 0;

	unsigned long tableParams = m_rootDescriptorTablesBitMap;
	unsigned long rootIndex;
	while (_BitScanForward(&rootIndex, tableParams))
	{
		tableParams ^= (1 << rootIndex);
		if (m_rootDescriptorTable[rootIndex].assignedHandlesBitMap != 0)
		{
			m_staleRootParamsBitMap |= (1 << rootIndex);
		}
	}
}


void DynamicDescriptorHeap::DescriptorHandleCache::StageDescriptorHandles(uint32_t rootIndex, uint32_t offset, uint32_t numHandles, const D3D12_CPU_DESCRIPTOR_HANDLE handles[])
{
	assert_msg(((1 << rootIndex) & m_rootDescriptorTablesBitMap) != 0, "Root parameter is not a CBV_SRV_UAV descriptor table");
	assert(offset + numHandles <= m_rootDescriptorTable[rootIndex].tableSize);

	DescriptorTableCache& tableCache = m_rootDescriptorTable[rootIndex];
	D3D12_CPU_DESCRIPTOR_HANDLE* copyDest = tableCache.tableStart + offset;
	for (uint32_t i = 0; i < numHandles; ++i)
	{
		copyDest[i] = handles[i];
	}
	tableCache.assignedHandlesBitMap |= ((1 << numHandles) - 1) << offset;
	m_staleRootParamsBitMap |= (1 << rootIndex);
}


void DynamicDescriptorHeap::DescriptorHandleCache::ParseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE type, const RootSignature& rootSig)
{
	uint32_t currentOffset = 0;

	assert_msg(rootSig.GetNumRootParameters() <= 16, "Maybe we need to support something greater");

	m_staleRootParamsBitMap = 0;
	m_rootDescriptorTablesBitMap = (type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ?
		rootSig.GetSamplerTableBitmap() : rootSig.GetDescriptorTableBitmap());

	unsigned long tableParams = m_rootDescriptorTablesBitMap;
	unsigned long rootIndex;
	const vector<uint32_t>& descriptorTableSize = rootSig.GetDescriptorTableSizes();
	while (_BitScanForward(&rootIndex, tableParams))
	{
		tableParams ^= (1 << rootIndex);

		uint32_t tableSize = descriptorTableSize[rootIndex];
		assert(tableSize > 0);

		DescriptorTableCache& rootDescriptorTable = m_rootDescriptorTable[rootIndex];
		rootDescriptorTable.assignedHandlesBitMap = 0;
		rootDescriptorTable.tableStart = m_handleCache + currentOffset;
		rootDescriptorTable.tableSize = tableSize;

		currentOffset += tableSize;
	}

	m_maxCachedDescriptors = currentOffset;

	assert_msg(m_maxCachedDescriptors <= kMaxNumDescriptors, "Exceeded user-supplied maximum cache size");
}

} // namespace Luna::DX12