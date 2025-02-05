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


namespace Luna::VK
{

ColorBufferVK::ColorBufferVK(const ColorBufferDesc& desc, const ColorBufferDescExt& descExt)
	: m_name{ desc.name }
	, m_resourceType{ desc.resourceType }
	, m_usageState{ descExt.usageState }
	, m_width{ desc.width }
	, m_height{ desc.height }
	, m_arraySizeOrDepth{ desc.arraySizeOrDepth }
	, m_numMips{ desc.numMips }
	, m_numSamples{ desc.numSamples }
	, m_format{ desc.format }
	, m_clearColor{ desc.clearColor }
	, m_image{ descExt.image }
	, m_imageViewRtv{ descExt.imageViewRtv }
	, m_imageViewSrv{ descExt.imageViewSrv }
	, m_imageInfoSrv{ descExt.imageInfoSrv }
	, m_imageInfoUav{ descExt.imageInfoUav }
{}


NativeObjectPtr ColorBufferVK::GetNativeObject(NativeObjectType type, uint32_t index) const noexcept
{
	switch (type)
	{
	case NativeObjectType::VK_Image:
		return NativeObjectPtr(m_image->Get());

	case NativeObjectType::VK_ImageView_RTV:
		return NativeObjectPtr(m_imageViewRtv->Get());

	case NativeObjectType::VK_ImageView_SRV:
		return NativeObjectPtr(m_imageViewSrv->Get());

	case NativeObjectType::VK_ImageInfo_SRV:
		return NativeObjectPtr(GetImageInfoSrv());

	case NativeObjectType::VK_ImageInfo_UAV:
		return NativeObjectPtr(GetImageInfoUav());

	default:
		assert(false);
		return nullptr;
	}
}


TextureDimension ColorBufferVK::GetDimension() const noexcept
{
	return ResourceTypeToTextureDimension(m_resourceType);
}

} // namespace Luna::VK