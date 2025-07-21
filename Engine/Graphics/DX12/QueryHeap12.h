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

#include "Graphics\QueryHeap.h"
#include "Graphics\DX12\DirectXCommon.h"


namespace Luna::DX12
{

// Forward declaration
class Device;


class QueryHeap : public IQueryHeap
{
	friend class Device;

public:
	ID3D12QueryHeap* GetQueryHeap() const noexcept { return m_heap.get(); }

	D3D12_QUERY_TYPE GetQueryType() const noexcept
	{
		switch (GetType())
		{
		case QueryHeapType::Occlusion:
			return D3D12_QUERY_TYPE_OCCLUSION;
		case QueryHeapType::Timestamp:
			return D3D12_QUERY_TYPE_TIMESTAMP;
		case QueryHeapType::PipelineStats:
			return D3D12_QUERY_TYPE_PIPELINE_STATISTICS1;
		default:
			return D3D12_QUERY_TYPE_OCCLUSION;
		}
	}

protected:
	Device* m_device{ nullptr };

	wil::com_ptr<ID3D12QueryHeap> m_heap;
};

} // namespace Luna::DX12