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

void DescriptorSet::Initialize(const RootSignature& rootSignature, uint32_t rootParamIndex)
{
	m_handle = rootSignature.CreateDescriptorSet(rootParamIndex);
}


void DescriptorSet::SetSRV(int slot, const ColorBuffer& colorBuffer)
{
	GetResourceManager()->SetSRV(m_handle.get(), slot, colorBuffer);
}


void DescriptorSet::SetSRV(int slot, const DepthBuffer& depthBuffer, bool depthSrv)
{
	GetResourceManager()->SetSRV(m_handle.get(), slot, depthBuffer, depthSrv);
}


void DescriptorSet::SetSRV(int slot, const GpuBuffer& gpuBuffer)
{
	GetResourceManager()->SetSRV(m_handle.get(), slot, gpuBuffer);
}


void DescriptorSet::SetUAV(int slot, const ColorBuffer& colorBuffer, uint32_t uavIndex)
{
	GetResourceManager()->SetUAV(m_handle.get(), slot, colorBuffer, uavIndex);
}


void DescriptorSet::SetUAV(int slot, const DepthBuffer& depthBuffer)
{
	GetResourceManager()->SetUAV(m_handle.get(), slot, depthBuffer);
}


void DescriptorSet::SetUAV(int slot, const GpuBuffer& gpuBuffer)
{
	GetResourceManager()->SetUAV(m_handle.get(), slot, gpuBuffer);
}


void DescriptorSet::SetCBV(int slot, const GpuBuffer& gpuBuffer)
{
	GetResourceManager()->SetCBV(m_handle.get(), slot, gpuBuffer);
}


void DescriptorSet::SetDynamicOffset(uint32_t offset)
{
	GetResourceManager()->SetDynamicOffset(m_handle.get(), offset);
}

} // namespace Luna