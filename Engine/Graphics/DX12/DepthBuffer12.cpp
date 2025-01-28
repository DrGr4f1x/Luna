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

DepthBuffer12::DepthBuffer12(const DepthBufferDesc& depthBufferDesc, const DepthBufferDescExt& depthBufferDescExt)
	: m_name{ depthBufferDesc.name }
	, m_resourceType{ depthBufferDesc.resourceType }
	, m_usageState{ depthBufferDescExt.usageState }
	, m_width{ depthBufferDesc.width }
	, m_height{ depthBufferDesc.height }
	, m_arraySizeOrDepth{ depthBufferDesc.arraySizeOrDepth }
	, m_numMips{ depthBufferDesc.numMips }
	, m_numSamples{ depthBufferDesc.numSamples }
	, m_planeCount{ depthBufferDescExt.planeCount }
	, m_format{ depthBufferDesc.format }
	, m_clearDepth{ depthBufferDesc.clearDepth }
	, m_clearStencil{ depthBufferDesc.clearStencil }
	, m_resource{ depthBufferDescExt.resource }
	, m_dsvHandles{ depthBufferDescExt.dsvHandles }
	, m_depthSrvHandle{ depthBufferDescExt.depthSrvHandle }
	, m_stencilSrvHandle{ depthBufferDescExt.stencilSrvHandle }
{}


NativeObjectPtr DepthBuffer12::GetNativeObject(NativeObjectType type, uint32_t index) const noexcept
{
	switch (type)
	{
	case NativeObjectType::DX12_Resource:
		return NativeObjectPtr(m_resource.get());

	case NativeObjectType::DX12_DSV:
		return NativeObjectPtr(m_dsvHandles[0].ptr);

	case NativeObjectType::DX12_DSV_ReadOnly:
		return NativeObjectPtr(m_dsvHandles[1].ptr);

	case NativeObjectType::DX12_DSV_DepthReadOnly:
		return NativeObjectPtr(m_dsvHandles[2].ptr);

	case NativeObjectType::DX12_DSV_StencilReadOnly:
		return NativeObjectPtr(m_dsvHandles[3].ptr);

	case NativeObjectType::DX12_SRV_Depth:
		return NativeObjectPtr(m_depthSrvHandle.ptr);

	case NativeObjectType::DX12_SRV_Stencil:
		return NativeObjectPtr(m_stencilSrvHandle.ptr);

	default:
		assert(false);
		return nullptr;
	}
}


TextureDimension DepthBuffer12::GetDimension() const noexcept
{
	return ResourceTypeToTextureDimension(m_resourceType);
}

} // namespace Luna::DX12