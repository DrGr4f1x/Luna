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

#include "RayTracingPipeline12.h"


namespace Luna::DX12
{

const wchar_t* RayTracingPipeline::GetHitGroupName(uint32_t index) const
{
	assert(index < (uint32_t)m_hitGroupNames.size());
	return m_hitGroupNames[index].c_str();
}

} // namespace Luna::DX12