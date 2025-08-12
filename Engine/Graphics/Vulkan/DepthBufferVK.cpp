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


namespace Luna::VK
{

const IDescriptor* DepthBuffer::GetDsvDescriptor(DepthStencilAspect depthStencilAspect) const noexcept
{ 
	switch (depthStencilAspect)
	{
	case DepthStencilAspect::DepthReadOnly:		return &m_depthOnlyDescriptor;
	case DepthStencilAspect::StencilReadOnly:	return &m_stencilOnlyDescriptor;
	default:									return &m_depthStencilDescriptor;
	}
}


const IDescriptor* DepthBuffer::GetSrvDescriptor(bool depthSrv) const noexcept
{
	return depthSrv ? &m_depthOnlyDescriptor : &m_stencilOnlyDescriptor;
}

} // namespace Luna::VK