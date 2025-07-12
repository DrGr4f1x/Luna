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

protected:
	QueryHeapDesc m_desc{};
};

using QueryHeapPtr = std::shared_ptr<IQueryHeap>;

} // namespace Luna