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

#include "ResourceSet.h"

#include "RootSignature.h"


namespace Luna
{

void ResourceSet::Initialize(const IRootSignature* rootSignature)
{
	const uint32_t numRootParameters = rootSignature->GetNumRootParameters();
	
	m_descriptorSets.reserve(numRootParameters);
	m_descriptorSets.clear();

	for (uint32_t i = 0; i < numRootParameters; ++i)
	{
		DescriptorSetPtr descriptorSet = rootSignature->CreateDescriptorSet(i);
		
		m_descriptorSets.push_back(descriptorSet);
	}
}


void ResourceSet::SetSRV(int param, int slot, const IColorBuffer* colorBuffer)
{
	assert(param < (int)m_descriptorSets.size());
	m_descriptorSets[param]->SetSRV(slot, colorBuffer);
}


void ResourceSet::SetSRV(int param, int slot, const IDepthBuffer* depthBuffer, bool depthSrv)
{
	assert(param < (int)m_descriptorSets.size());
	m_descriptorSets[param]->SetSRV(slot, depthBuffer, depthSrv);
}


void ResourceSet::SetSRV(int param, int slot, const IGpuBuffer* gpuBuffer)
{
	assert(param < (int)m_descriptorSets.size());
	m_descriptorSets[param]->SetSRV(slot, gpuBuffer);
}


void ResourceSet::SetUAV(int param, int slot, const IColorBuffer* colorBuffer, uint32_t uavIndex)
{
	assert(param < (int)m_descriptorSets.size());
	m_descriptorSets[param]->SetUAV(slot, colorBuffer, uavIndex);
}


void ResourceSet::SetUAV(int param, int slot, const IDepthBuffer* depthBuffer)
{
	assert(param < (int)m_descriptorSets.size());
	m_descriptorSets[param]->SetUAV(slot, depthBuffer);
}


void ResourceSet::SetUAV(int param, int slot, const IGpuBuffer* gpuBuffer)
{
	assert(param < (int)m_descriptorSets.size());
	m_descriptorSets[param]->SetUAV(slot, gpuBuffer);
}


void ResourceSet::SetCBV(int param, int slot, const IGpuBuffer* gpuBuffer)
{
	assert(param < (int)m_descriptorSets.size());
	m_descriptorSets[param]->SetCBV(slot, gpuBuffer);
}


void ResourceSet::SetDynamicOffset(int param, uint32_t offset)
{
	assert(param < (int)m_descriptorSets.size());
	m_descriptorSets[param]->SetDynamicOffset(offset);
}


DescriptorSetPtr& ResourceSet::operator[](uint32_t index)
{
	assert(index < (int)m_descriptorSets.size());
	return m_descriptorSets[index];
}


const DescriptorSetPtr& ResourceSet::operator[](uint32_t index) const
{
	assert(index < (int)m_descriptorSets.size());
	return m_descriptorSets[index];
}

} // namespace Luna