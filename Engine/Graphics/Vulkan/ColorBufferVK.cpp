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

#include "DeviceVK.h"


namespace Luna::VK
{

const IDescriptor* ColorBuffer::GetUavDescriptor(uint32_t index) const noexcept
{
	assert(index == 0);
	return &m_uavDescriptor;
}


void ColorBuffer::InitializeDescriptors(Device* device, bool initUavDescriptor)
{
	const bool useDescriptorHeap = device->UseDescriptorHeap();

	m_srvDescriptor.ReadSampledImageDescriptor(
		device,
		m_srvImageView->GetCreateInfo(),
		m_srvImageView->Get());

	if (initUavDescriptor)
	{
		m_uavDescriptor.ReadStorageImageDescriptor(
			device,
			m_srvImageView->GetCreateInfo(),
			m_srvImageView->Get());
	}
}

} // namespace Luna::VK