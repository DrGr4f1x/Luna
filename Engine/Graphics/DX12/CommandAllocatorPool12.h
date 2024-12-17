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


namespace Luna::DX12
{

class CommandAllocatorPool
{
public:
	CommandAllocatorPool(ID3D12Device* device, D3D12_COMMAND_LIST_TYPE commandListType);
	~CommandAllocatorPool() = default;

	ID3D12CommandAllocator* RequestAllocator(uint64_t completedFenceValue);
	void DiscardAllocator(uint64_t fenceValue, ID3D12CommandAllocator* allocator);

	inline size_t Size() const noexcept { return m_allocatorPool.size(); }

private:
	ID3D12Device* m_dxDevice{ nullptr };
	const D3D12_COMMAND_LIST_TYPE m_commandListType;

	std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>> m_allocatorPool;
	std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> m_readyAllocators;
	std::mutex m_allocatorMutex;
};

} // namespace Luna::DX12