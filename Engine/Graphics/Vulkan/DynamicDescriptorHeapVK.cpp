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

#include "DynamicDescriptorHeapVK.h"

using namespace std;


namespace Luna::VK
{

void DefaultDynamicDescriptorHeap::CleanupUsedPools(uint64_t fenceValue)
{
	RetireCurrentPools();
	RetireUsedPools(fenceValue);
}


void DefaultDynamicDescriptorHeap::RetireCurrentPools()
{
	for (uint32_t i = 0; i < m_descriptorPools.size(); ++i)
	{
		auto pool = m_descriptorPools[i];
		m_retiredPools.push_back(pool);
		m_descriptorPools[i].reset();
	}
}


void DefaultDynamicDescriptorHeap::RetireUsedPools(uint64_t fenceValue)
{
	lock_guard<mutex> lockGuard(sm_mutex);
	for (auto iter = m_retiredPools.begin(); iter != m_retiredPools.end(); ++iter)
	{
		sm_retiredDescriptorPools.push(make_pair(fenceValue, *iter));
	}
	m_retiredPools.clear();
}

} // namespace Luna::VK