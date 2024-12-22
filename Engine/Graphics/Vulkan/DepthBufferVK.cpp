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

DepthBuffer::DepthBuffer(const DepthBufferDesc& desc, const DepthBufferDescExt& descExt)
	: m_name{ desc.name }
	, m_image{ descExt.image }
	, m_usageState{ descExt.usageState }
	, m_resourceType{ desc.resourceType }
	, m_width{ desc.width }
	, m_height{ desc.height }
	, m_arraySizeOrDepth{ desc.arraySizeOrDepth }
	, m_numMips{ desc.numMips }
	, m_numSamples{ desc.numSamples }
	, m_format{ desc.format }
	, m_clearDepth{ desc.clearDepth }
	, m_clearStencil{ desc.clearStencil }
	, m_imageViewDepthStencil{ descExt.imageViewDepthStencil }
	, m_imageViewDepthOnly{ descExt.imageViewDepthOnly }
	, m_imageViewStencilOnly{ descExt.imageViewStencilOnly }
	, m_imageInfoDepth{ descExt.imageInfoDepth }
	, m_imageInfoStencil{ descExt.imageInfoStencil }
{
	m_numMips = m_numMips == 0 ? ComputeNumMips(m_width, m_height) : m_numMips;
}


NativeObjectPtr DepthBuffer::GetNativeObject(NativeObjectType nativeObjectType) const noexcept
{
	using enum NativeObjectType;

	switch (nativeObjectType)
	{
	case VK_Image:
		return NativeObjectPtr(m_image->Get());
		break;
	default:
		return nullptr;
		break;
	}
}

} // namespace Luna::VK