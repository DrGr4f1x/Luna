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

#include "ResourceSetVK.h"

#include "Graphics\Vulkan\DescriptorSetVK.h"


namespace Luna::VK
{

ResourceSet::ResourceSet(const std::array<wil::com_ptr<IDescriptorSet>, MaxRootParameters>& descriptorSets)
{
	for (uint32_t i = 0; i < MaxRootParameters; ++i)
	{
		wil::com_ptr<IDescriptorSetVK> descriptorSet12;
		m_descriptorSets[i] = descriptorSets[i].query<IDescriptorSetVK>();
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


IDescriptorSetVK* ResourceSet::GetDescriptorSet(uint32_t index)
{
	assert(index < MaxRootParameters);
	return m_descriptorSets[index].get();
}


const IDescriptorSetVK* ResourceSet::GetDescriptorSet(uint32_t index) const
{
	assert(index < MaxRootParameters);
	return m_descriptorSets[index].get();
}

} // namespace Luna::VK