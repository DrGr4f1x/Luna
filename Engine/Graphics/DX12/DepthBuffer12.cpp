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

#include "DepthBuffer12.h"


namespace Luna::DX12
{

DepthBuffer::DepthBuffer(Device* device)
{
	for (auto& dsvDescriptor : m_dsvDescriptors)
	{
		dsvDescriptor.SetDevice(device);
	}
	m_depthSrvDescriptor.SetDevice(device);
	m_stencilSrvDescriptor.SetDevice(device);
}


const IDescriptor* DepthBuffer::GetDsvDescriptor(DepthStencilAspect depthStencilAspect) const noexcept
{
	switch (depthStencilAspect)
	{
	case DepthStencilAspect::ReadWrite:		return &m_dsvDescriptors[0];
	case DepthStencilAspect::ReadOnly:		return &m_dsvDescriptors[1];
	case DepthStencilAspect::DepthReadOnly:	return &m_dsvDescriptors[2];
	default:								return &m_dsvDescriptors[3];
	}
}


const IDescriptor* DepthBuffer::GetSrvDescriptor(bool depthSrv) const noexcept
{
	return depthSrv ? &m_depthSrvDescriptor : &m_stencilSrvDescriptor;
}

} // namespace Luna::DX12