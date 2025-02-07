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

#include "ResourceSet12.h"

#include "Graphics\DX12\DescriptorSet12.h"


namespace Luna::DX12
{

ResourceSet::ResourceSet(const std::vector<wil::com_ptr<IDescriptorSet>>& descriptorSets)
{
	m_descriptorSets.reserve(descriptorSets.size());
	for (uint32_t i = 0; i < (uint32_t)descriptorSets.size(); ++i)
	{
		wil::com_ptr<IDescriptorSet12> descriptorSet12;
		m_descriptorSets.push_back(descriptorSets[i].query<IDescriptorSet12>());
	}
}


void ResourceSet::SetSRV(int param, int slot, const IColorBuffer* colorBuffer)
{
	GetDescriptorSet(param)->SetSRV(slot, colorBuffer);
}


void ResourceSet::SetSRV(int param, int slot, const IDepthBuffer* depthBuffer, bool depthSrv)
{
	GetDescriptorSet(param)->SetSRV(slot, depthBuffer, depthSrv);
}


void ResourceSet::SetSRV(int param, int slot, const IGpuBuffer* gpuBuffer)
{
	GetDescriptorSet(param)->SetSRV(slot, gpuBuffer);
}


void ResourceSet::SetUAV(int param, int slot, const IColorBuffer* colorBuffer, uint32_t uavIndex)
{
	GetDescriptorSet(param)->SetUAV(slot, colorBuffer, uavIndex);
}


void ResourceSet::SetUAV(int param, int slot, const IDepthBuffer* depthBuffer)
{
	GetDescriptorSet(param)->SetUAV(slot, depthBuffer);
}


void ResourceSet::SetUAV(int param, int slot, const IGpuBuffer* gpuBuffer)
{
	GetDescriptorSet(param)->SetUAV(slot, gpuBuffer);
}


void ResourceSet::SetCBV(int param, int slot, const IGpuBuffer* gpuBuffer)
{
	GetDescriptorSet(param)->SetCBV(slot, gpuBuffer);
}


void ResourceSet::SetDynamicOffset(int param, uint32_t offset)
{
	GetDescriptorSet(param)->SetDynamicOffset(offset);
}


IDescriptorSet12* ResourceSet::GetDescriptorSet(uint32_t index)
{
	assert(index < (uint32_t)m_descriptorSets.size());
	return m_descriptorSets[index].get();
}


const IDescriptorSet12* ResourceSet::GetDescriptorSet(uint32_t index) const
{
	assert(index < (uint32_t)m_descriptorSets.size());
	return m_descriptorSets[index].get();
}

} // namespace Luna::DX12