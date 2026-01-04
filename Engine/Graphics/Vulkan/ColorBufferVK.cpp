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

#include "ColorBufferVK.h"


namespace Luna::VK
{

const IDescriptor* ColorBuffer::GetUavDescriptor(uint32_t index) const noexcept
{
	assert(index == 0);
	return &m_uavDescriptor;
}

} // namespace Luna::VK