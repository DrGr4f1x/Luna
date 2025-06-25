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

#include "Graphics\DX12\DescriptorAllocator12.h"
#include "Graphics\DX12\DirectXCommon.h"


namespace Luna::DX12
{

// Forward declarations
class CommandContext12;
class RootSignature;


class DynamicDescriptorHeap
{
public:
	DynamicDescriptorHeap(CommandContext12& owningContext, D3D12_DESCRIPTOR_HEAP_TYPE heapType);
	~DynamicDescriptorHeap() = default;

	static void DestroyAll()
	{
		sm_descriptorHeapPool[0].clear();
		sm_descriptorHeapPool[1].clear();
	}

	void CleanupUsedHeaps(uint64_t fenceValue);

	// Copy multiple handles into the cache area reserved for the specified root parameter.
	void SetGraphicsDescriptorHandles(uint32_t rootIndex, uint32_t offset, uint32_t numHandles, const D3D12_CPU_DESCRIPTOR_HANDLE handles[])
	{
		m_graphicsHandleCache.StageDescriptorHandles(rootIndex, offset, numHandles, handles);
	}

	void SetComputeDescriptorHandles(uint32_t rootIndex, uint32_t offset, uint32_t numHandles, const D3D12_CPU_DESCRIPTOR_HANDLE handles[])
	{
		m_computeHandleCache.StageDescriptorHandles(rootIndex, offset, numHandles, handles);
	}

	// Bypass the cache and upload directly to the shader-visible heap
	D3D12_GPU_DESCRIPTOR_HANDLE UploadDirect(D3D12_CPU_DESCRIPTOR_HANDLE handles);

	// Deduce cache layout needed to support the descriptor tables needed by the root signature.
	void ParseGraphicsRootSignature(const RootSignature& rootSig)
	{
		m_graphicsHandleCache.ParseRootSignature(m_descriptorType, rootSig);
	}

	void ParseComputeRootSignature(const RootSignature& rootSig)
	{
		m_computeHandleCache.ParseRootSignature(m_descriptorType, rootSig);
	}

	// Upload any new descriptors in the cache to the shader-visible heap.
	inline void CommitGraphicsRootDescriptorTables(ID3D12GraphicsCommandList* cmdList)
	{
		if (m_graphicsHandleCache.m_staleRootParamsBitMap != 0)
		{
			CopyAndBindStagedTables(m_graphicsHandleCache, cmdList, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
		}
	}

	inline void CommitComputeRootDescriptorTables(ID3D12GraphicsCommandList* cmdList)
	{
		if (m_computeHandleCache.m_staleRootParamsBitMap != 0)
		{
			CopyAndBindStagedTables(m_computeHandleCache, cmdList, &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
		}
	}

private:
	// Static methods
	static ID3D12DescriptorHeap* RequestDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType);
	static void DiscardDescriptorHeaps(D3D12_DESCRIPTOR_HEAP_TYPE heapType, uint64_t fenceValueForReset, const std::vector<ID3D12DescriptorHeap*>& usedHeaps);

	bool HasSpace(uint32_t count)
	{
		return (m_currentHeapPtr != nullptr && m_currentOffset + count <= NumDescriptorsPerHeap);
	}

	void RetireCurrentHeap();
	void RetireUsedHeaps(uint64_t fenceValue);
	ID3D12DescriptorHeap* GetHeapPointer();

	DescriptorHandle Allocate(uint32_t count)
	{
		DescriptorHandle ret = m_firstDescriptor + m_currentOffset * m_descriptorSize;
		m_currentOffset += count;
		return ret;
	}

	struct DescriptorHandleCache;

	void CopyAndBindStagedTables(DescriptorHandleCache& handleCache, ID3D12GraphicsCommandList* cmdList,
		void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE));

	// Mark all descriptors in the cache as stale and in need of re-uploading.
	void UnbindAllValid();

private:
	// Static members
	constexpr static uint32_t NumDescriptorsPerHeap{ 1024 };
	static std::mutex sm_mutex;
	static std::vector<wil::com_ptr<ID3D12DescriptorHeap>> sm_descriptorHeapPool[2];
	static std::queue<std::pair<uint64_t, ID3D12DescriptorHeap*>> sm_retiredDescriptorHeaps[2];
	static std::queue<ID3D12DescriptorHeap*> sm_availableDescriptorHeaps[2];

	// Non-static members
	CommandContext12& m_owningContext;
	ID3D12DescriptorHeap* m_currentHeapPtr;
	const D3D12_DESCRIPTOR_HEAP_TYPE m_descriptorType;
	uint32_t m_descriptorSize;
	uint32_t m_currentOffset;
	DescriptorHandle m_firstDescriptor;
	std::vector<ID3D12DescriptorHeap*> m_retiredHeaps;

	// Describes a descriptor table entry:  a region of the handle cache and which handles have been set
	struct DescriptorTableCache
	{
		uint32_t assignedHandlesBitMap{ 0 };
		D3D12_CPU_DESCRIPTOR_HANDLE* tableStart{ nullptr };
		uint32_t tableSize{ 0 };
	};

	struct DescriptorHandleCache
	{
		DescriptorHandleCache()
		{
			ClearCache();
		}

		void ClearCache()
		{
			m_rootDescriptorTablesBitMap = 0;
			m_maxCachedDescriptors = 0;
		}

		uint32_t m_rootDescriptorTablesBitMap;
		uint32_t m_staleRootParamsBitMap;
		uint32_t m_maxCachedDescriptors;

		static const uint32_t kMaxNumDescriptors = 256;
		static const uint32_t kMaxNumDescriptorTables = 16;

		uint32_t ComputeStagedSize();
		void CopyAndBindStaleTables(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t descriptorSize, DescriptorHandle destHandleStart, ID3D12GraphicsCommandList* cmdList,
			void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::* SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE));

		DescriptorTableCache m_rootDescriptorTable[kMaxNumDescriptorTables];
		D3D12_CPU_DESCRIPTOR_HANDLE m_handleCache[kMaxNumDescriptors];

		void UnbindAllValid();
		void StageDescriptorHandles(uint32_t rootIndex, uint32_t offset, uint32_t numHandles, const D3D12_CPU_DESCRIPTOR_HANDLE handles[]);
		void ParseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE type, const RootSignature& rootSig);
	};

	DescriptorHandleCache m_graphicsHandleCache;
	DescriptorHandleCache m_computeHandleCache;
};

} // namespace Luna::DX12