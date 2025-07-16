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


namespace Luna
{

uint32_t IRootSignature::GetNumRootParameters() const noexcept
{
	return (uint32_t)m_desc.rootParameters.size();
}


const RootParameter& IRootSignature::GetRootParameter(uint32_t index) const noexcept
{
	assert(index < (uint32_t)m_desc.rootParameters.size());
	return m_desc.rootParameters[index];
}

} // using namespace Luna