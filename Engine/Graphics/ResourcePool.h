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

#include <queue>


namespace Luna
{

template <class TFactory, typename TDesc, typename TData, uint32_t MaxItems>
class ResourcePool1
{
public:
	ResourcePool1()
	{
		for (uint32_t i = 0; i < MaxItems; ++i)
		{
			m_freeList.push(i);
			m_descs[i] = TDesc{};
			m_dataItems[i] = TData{};
		}
	}

protected:
	uint32_t AllocateIndex(const TDesc& desc)
	{
		std::lock_guard guard(m_allocationMutex);

		assert(!m_freeList.empty());

		uint32_t index = m_freeList.front();
		m_freeList.pop();

		m_descs[index] = desc;

		m_dataItems[index] = m_factory.Create(desc);

		return index;
	}


	void FreeIndex(uint32_t index)
	{
		std::lock_guard guard(m_allocationMutex);

		// TODO: queue this up to execute in one big batch, e.g. at the end of the frame
		m_descs[index] = TDesc{};
		m_dataItems[index] = TData{};

		m_freeList.push(index);
	}

protected:
	// Allocation mutex
	std::mutex m_allocationMutex;

	// Free list
	std::queue<uint32_t> m_freeList;

	// Resource descriptions
	std::array<TDesc, MaxItems> m_descs;

	// Data items
	std::array<TData, MaxItems> m_dataItems;

	// Handle factory
	TFactory m_factory;
};

} // namespace Luna