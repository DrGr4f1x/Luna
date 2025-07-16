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

D3D12_CPU_DESCRIPTOR_HANDLE DepthBuffer::GetSrvHandle(bool depthSrv) const noexcept
{
	return depthSrv ? m_depthSrvHandle : m_stencilSrvHandle;
}


D3D12_CPU_DESCRIPTOR_HANDLE DepthBuffer::GetDsvHandle(DepthStencilAspect depthStencilAspect) const noexcept
{
	switch (depthStencilAspect)
	{
	case DepthStencilAspect::ReadWrite:		return m_dsvHandles[0];
	case DepthStencilAspect::ReadOnly:		return m_dsvHandles[1];
	case DepthStencilAspect::DepthReadOnly:	return m_dsvHandles[2];
	default:								return m_dsvHandles[3];
	}
}

} // namespace Luna::DX12