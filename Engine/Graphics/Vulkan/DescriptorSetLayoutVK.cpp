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

#include "DescriptorSetLayoutVK.h"


namespace Luna::VK
{

BindingInfo DescriptorBindingTemplate::GetBindingInfo(uint32_t bindingIndex) const noexcept
{
	auto res = m_bindingInfoMap.find(bindingIndex);
	assert(res != m_bindingInfoMap.end());
	return res->second;
}


wil::com_ptr<CVkDescriptorSetLayout> DescriptorSetLayout::GetDescriptorSetLayout() const noexcept
{
	return m_descriptorSetLayout;
}


VkDeviceSize DescriptorSetLayout::GetBindingOffset(uint32_t bindingIndex) const
{
	return m_bindingTemplate->GetBindingInfo(bindingIndex).offset;
}


HeapDescriptorBindingMapCompact::HeapDescriptorBindingMapCompact()
{
	for (uint32_t i = 0; i < 32; ++i)
	{
		m_srvRegisterOffsetMap[i] = ~0u;
		m_uavRegisterOffsetMap[i] = ~0u;
		m_cbvRegisterOffsetMap[i] = ~0u;
		m_samplerRegisterOffsetMap[i] = ~0u;
	}
}


uint32_t HeapDescriptorBindingMapCompact::GetRegisterOffsetSRV(uint32_t srvRegister) const
{
	assert(srvRegister < m_srvRegisterOffsetMap.size());
	return m_srvRegisterOffsetMap[srvRegister];
}


uint32_t HeapDescriptorBindingMapCompact::GetRegisterOffsetUAV(uint32_t uavRegister) const
{
	assert(uavRegister < m_uavRegisterOffsetMap.size());
	return m_uavRegisterOffsetMap[uavRegister];
}


uint32_t HeapDescriptorBindingMapCompact::GetRegisterOffsetCBV(uint32_t cbvRegister) const
{
	assert(cbvRegister < m_cbvRegisterOffsetMap.size());
	return m_cbvRegisterOffsetMap[cbvRegister];
}


uint32_t HeapDescriptorBindingMapCompact::GetRegisterOffsetSampler(uint32_t samplerRegister) const
{
	assert(samplerRegister < m_samplerRegisterOffsetMap.size());
	return m_samplerRegisterOffsetMap[samplerRegister];
}


void HeapDescriptorBindingMapCompact::SetRegisterOffsetSRV(uint32_t srvRegister, uint32_t offset)
{
	assert(srvRegister < m_srvRegisterOffsetMap.size());
	m_srvRegisterOffsetMap[srvRegister] = offset;
}


void HeapDescriptorBindingMapCompact::SetRegisterOffsetUAV(uint32_t uavRegister, uint32_t offset)
{
	assert(uavRegister < m_uavRegisterOffsetMap.size());
	m_uavRegisterOffsetMap[uavRegister] = offset;
}


void HeapDescriptorBindingMapCompact::SetRegisterOffsetCBV(uint32_t cbvRegister, uint32_t offset)
{
	assert(cbvRegister < m_cbvRegisterOffsetMap.size());
	m_cbvRegisterOffsetMap[cbvRegister] = offset;
}


void HeapDescriptorBindingMapCompact::SetRegisterOffsetSampler(uint32_t samplerRegister, uint32_t offset)
{
	assert(samplerRegister < m_samplerRegisterOffsetMap.size());
	m_samplerRegisterOffsetMap[samplerRegister] = offset;
}


uint32_t HeapDescriptorBindingMapGeneral::GetRegisterOffsetSRV(uint32_t srvRegister) const
{
	auto it = m_srvRegisterOffsetMap.find(srvRegister);
	assert(it != m_srvRegisterOffsetMap.end());
	return it->second;
}


uint32_t HeapDescriptorBindingMapGeneral::GetRegisterOffsetUAV(uint32_t uavRegister) const
{
	auto it = m_uavRegisterOffsetMap.find(uavRegister);
	assert(it != m_uavRegisterOffsetMap.end());
	return it->second;
}


uint32_t HeapDescriptorBindingMapGeneral::GetRegisterOffsetCBV(uint32_t cbvRegister) const
{
	auto it = m_cbvRegisterOffsetMap.find(cbvRegister);
	assert(it != m_cbvRegisterOffsetMap.end());
	return it->second;
}


uint32_t HeapDescriptorBindingMapGeneral::GetRegisterOffsetSampler(uint32_t samplerRegister) const
{
	auto it = m_samplerRegisterOffsetMap.find(samplerRegister);
	assert(it != m_samplerRegisterOffsetMap.end());
	return it->second;
}


void HeapDescriptorBindingMapGeneral::SetRegisterOffsetSRV(uint32_t srvRegister, uint32_t offset)
{
	m_srvRegisterOffsetMap[srvRegister] = offset;
}


void HeapDescriptorBindingMapGeneral::SetRegisterOffsetUAV(uint32_t uavRegister, uint32_t offset)
{
	m_uavRegisterOffsetMap[uavRegister] = offset;
}


void HeapDescriptorBindingMapGeneral::SetRegisterOffsetCBV(uint32_t cbvRegister, uint32_t offset)
{
	m_cbvRegisterOffsetMap[cbvRegister] = offset;
}


void HeapDescriptorBindingMapGeneral::SetRegisterOffsetSampler(uint32_t samplerRegister, uint32_t offset)
{
	m_samplerRegisterOffsetMap[samplerRegister] = offset;
}

} // namespace Luna::VK