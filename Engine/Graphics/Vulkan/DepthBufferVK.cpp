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

#include "DepthBufferVK.h"

#include "DeviceVK.h"


namespace Luna::VK
{

const IDescriptor* DepthBuffer::GetSrvDescriptor(bool depthSrv) const noexcept
{
	return depthSrv ? &m_depthOnlyDescriptor : &m_stencilOnlyDescriptor;
}


VkImageView DepthBuffer::GetDsvImageView(DepthStencilAspect aspect) const
{
	switch (aspect)
	{
	case DepthStencilAspect::DepthReadOnly:
		return m_depthOnlyImageView->Get();
		break;

	case DepthStencilAspect::StencilReadOnly:
		return m_stencilOnlyImageView->Get();
		break;

	default:
		return m_depthStencilImageView->Get();
		break;
	}
}

void DepthBuffer::InitializeDescriptors(Device* device)
{
	const bool useDescriptorHeap = device->UseDescriptorHeap();

	m_depthOnlyDescriptor.ReadSampledImageDescriptor(device, m_depthOnlyImageView->GetCreateInfo(), m_depthOnlyImageView->Get());
	m_stencilOnlyDescriptor.ReadSampledImageDescriptor(device, m_stencilOnlyImageView->GetCreateInfo(), m_stencilOnlyImageView->Get());
}

} // namespace Luna::VK