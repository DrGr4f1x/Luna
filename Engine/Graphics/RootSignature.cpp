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

#include "RootSignature.h"

#include "GraphicsCommon.h"
#include "DescriptorSet.h"
#include "ResourceManager.h"


namespace Luna
{

uint32_t RootSignature::GetNumRootParameters() const
{
	return (uint32_t)GetDesc().rootParameters.size();
}


const RootParameter& RootSignature::GetRootParameter(uint32_t index) const
{
	return GetDesc().rootParameters[index];
}


DescriptorSetHandle RootSignature::CreateDescriptorSet(uint32_t index) const
{
	return GetResourceManager()->CreateDescriptorSet(m_handle.get(), index);
}


void RootSignature::Initialize(RootSignatureDesc& desc)
{
	m_handle = GetResourceManager()->CreateRootSignature(desc);
}


const RootSignatureDesc& RootSignature::GetDesc() const
{
	return GetResourceManager()->GetRootSignatureDesc(m_handle.get());
}

} // using namespace Luna