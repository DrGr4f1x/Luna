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

DepthBufferData::DepthBufferData(const DepthBufferDescExt& descExt) noexcept
	: m_image{ descExt.image }
	, m_imageViewDepthStencil{ descExt.imageViewDepthStencil }
	, m_imageViewDepthOnly{ descExt.imageViewDepthOnly }
	, m_imageViewStencilOnly{ descExt.imageViewStencilOnly }
	, m_imageInfoDepth{ descExt.imageInfoDepth }
	, m_imageInfoStencil{ descExt.imageInfoStencil }
{}


DepthBufferVK::DepthBufferVK(const DepthBufferDesc& depthBufferDesc, const DepthBufferDescExt& depthBufferDescExt)
	: m_name{ depthBufferDesc.name }
	, m_resourceType{ depthBufferDesc.resourceType }
	, m_usageState{ depthBufferDescExt.usageState }
	, m_width{ depthBufferDesc.width }
	, m_height{ depthBufferDesc.height }
	, m_arraySizeOrDepth{ depthBufferDesc.arraySizeOrDepth }
	, m_numMips{ depthBufferDesc.numMips }
	, m_numSamples{ depthBufferDesc.numSamples }
	, m_format{ depthBufferDesc.format }
	, m_clearDepth{ depthBufferDesc.clearDepth }
	, m_clearStencil{ depthBufferDesc.clearStencil }
	, m_image{ depthBufferDescExt.image }
	, m_imageViewDepthStencil{ depthBufferDescExt.imageViewDepthStencil }
	, m_imageViewDepthOnly{ depthBufferDescExt.imageViewDepthOnly }
	, m_imageViewStencilOnly{ depthBufferDescExt.imageViewStencilOnly }
	, m_imageInfoDepth{ depthBufferDescExt.imageInfoDepth }
	, m_imageInfoStencil{ depthBufferDescExt.imageInfoStencil }
{}


NativeObjectPtr DepthBufferVK::GetNativeObject(NativeObjectType type, uint32_t index) const noexcept
{
	switch (type)
	{
	case NativeObjectType::VK_Image:
		return NativeObjectPtr(m_image->Get());

	default:
		assert(false);
		return nullptr;
	}
}


TextureDimension DepthBufferVK::GetDimension() const noexcept
{
	return ResourceTypeToTextureDimension(m_resourceType);
}

} // namespace Luna::VK