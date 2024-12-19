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

DepthBuffer::DepthBuffer(const DepthBufferDesc& desc, const DepthBufferDescExt& descExt) noexcept
	: m_name{ desc.name }
	, m_resource{ descExt.resource }
	, m_usageState{ descExt.usageState }
	, m_resourceType{ desc.resourceType }
	, m_width{ desc.width }
	, m_height{ desc.height }
	, m_arraySizeOrDepth{ desc.arraySizeOrDepth }
	, m_numMips{ desc.numMips }
	, m_numSamples{ desc.numSamples }
	, m_planeCount{ descExt.planeCount }
	, m_format{ desc.format }
	, m_clearDepth{ desc.clearDepth }
	, m_clearStencil{ desc.clearStencil }
	, m_dsvHandles{ descExt.dsvHandles }
	, m_depthSrvHandle{ descExt.depthSrvHandle }
	, m_stencilSrvHandle{ descExt.stencilSrvHandle }
{
	m_numMips = m_numMips == 0 ? ComputeNumMips(m_width, m_height) : m_numMips;
}


NativeObjectPtr DepthBuffer::GetNativeObject(NativeObjectType nativeObjectType) const noexcept
{
	if (nativeObjectType == NativeObjectType::DX12_Resource)
	{
		return NativeObjectPtr(m_resource.get());
	}

	return IGpuImage::GetNativeObject(nativeObjectType);
}

} // namespace Luna::DX12