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

#include "DescriptorSet.h"

#include "GraphicsCommon.h"
#include "RootSignature.h"


namespace Luna
{

DescriptorSetHandleType::~DescriptorSetHandleType()
{
	assert(m_pool);
	m_pool->DestroyHandle(this);
}


void DescriptorSet::Initialize(const RootSignature& rootSignature, uint32_t rootParamIndex)
{
	m_handle = rootSignature.CreateDescriptorSet(rootParamIndex);
}


void DescriptorSet::SetSRV(int slot, const IColorBuffer* colorBuffer)
{
	GetDescriptorSetPool()->SetSRV(m_handle.get(), slot, colorBuffer);
}


void DescriptorSet::SetSRV(int slot, const IDepthBuffer* depthBuffer, bool depthSrv)
{
	GetDescriptorSetPool()->SetSRV(m_handle.get(), slot, depthBuffer, depthSrv);
}


void DescriptorSet::SetSRV(int slot, const IGpuBuffer* gpuBuffer)
{
	GetDescriptorSetPool()->SetSRV(m_handle.get(), slot, gpuBuffer);
}


void DescriptorSet::SetUAV(int slot, const IColorBuffer* colorBuffer, uint32_t uavIndex)
{
	GetDescriptorSetPool()->SetUAV(m_handle.get(), slot, colorBuffer, uavIndex);
}


void DescriptorSet::SetUAV(int slot, const IDepthBuffer* depthBuffer)
{
	GetDescriptorSetPool()->SetUAV(m_handle.get(), slot, depthBuffer);
}


void DescriptorSet::SetUAV(int slot, const IGpuBuffer* gpuBuffer)
{
	GetDescriptorSetPool()->SetUAV(m_handle.get(), slot, gpuBuffer);
}


void DescriptorSet::SetCBV(int slot, const IGpuBuffer* gpuBuffer)
{
	GetDescriptorSetPool()->SetCBV(m_handle.get(), slot, gpuBuffer);
}


void DescriptorSet::SetDynamicOffset(uint32_t offset)
{
	GetDescriptorSetPool()->SetDynamicOffset(m_handle.get(), offset);
}

} // namespace Luna