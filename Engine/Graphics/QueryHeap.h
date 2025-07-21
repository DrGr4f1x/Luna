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

namespace Luna
{

// Note: This matches D3D12_QUERY_DATA_PIPELINE_STATISTICS1
struct PipelineStatistics
{
	uint64_t IAVertices{ 0 };
	uint64_t IAPrimitives{ 0 };
	uint64_t VSInvocations{ 0 };
	uint64_t GSInvocations{ 0 };
	uint64_t GSPrimitives{ 0 };
	uint64_t CInvocations{ 0 };
	uint64_t CPrimitives{ 0 };
	uint64_t PSInvocations{ 0 };
	uint64_t HSInvocations{ 0 };
	uint64_t DSInvocations{ 0 };
	uint64_t CSInvocations{ 0 };
	uint64_t ASInvocations{ 0 };
	uint64_t MSInvocations{ 0 };
	uint64_t MSPrimitives{ 0 };
};


struct QueryHeapDesc
{
	std::string name;
	QueryHeapType type{ QueryHeapType::Occlusion };
	uint32_t queryCount{ 1 };
};


class IQueryHeap
{
public:
	virtual ~IQueryHeap() = default;

	QueryHeapType GetType() const { return m_desc.type; }
	uint32_t GetQueryCount() const { return m_desc.queryCount; }
	size_t GetQuerySize() const noexcept 
	{ 
		return m_desc.type == QueryHeapType::PipelineStats ? sizeof(PipelineStatistics) : sizeof(uint64_t); 
	}

protected:
	QueryHeapDesc m_desc{};
};

using QueryHeapPtr = std::shared_ptr<IQueryHeap>;

} // namespace Luna