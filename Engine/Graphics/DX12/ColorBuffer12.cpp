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

#include "ColorBuffer12.h"

namespace Luna::DX12
{

D3D12_CPU_DESCRIPTOR_HANDLE ColorBuffer::GetUavHandle(uint32_t index) const
{
	assert(index < m_uavHandles.size());

	return m_uavHandles[index];
}

} // namespace Luna::DX12