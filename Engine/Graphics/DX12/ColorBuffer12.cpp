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

ColorBuffer::ColorBuffer(const ColorBufferDesc& desc, Device* device, ID3D12Resource* resource)
	: IColorBuffer{ desc }
	, m_resource{ resource }
{
	m_rtvDescriptor.SetDevice(device);
	m_srvDescriptor.SetDevice(device);
	for (auto& uavDescriptor : m_uavDescriptors)
	{
		uavDescriptor.SetDevice(device);
	}

	SetDebugName(m_resource.get(), desc.name);
}


const IDescriptor* ColorBuffer::GetUavDescriptor(uint32_t index) const noexcept
{
	assert(index < m_uavDescriptors.size());

	return &m_uavDescriptors[index];
}

} // namespace Luna::DX12