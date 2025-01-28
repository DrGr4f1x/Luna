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

ColorBufferData::ColorBufferData(const ColorBufferDescExt& descExt)
	: m_image{ descExt.image }
	, m_imageViewRtv{ descExt.imageViewRtv }
	, m_imageViewSrv{ descExt.imageViewSrv }
	, m_imageInfoSrv{ descExt.imageInfoSrv }
	, m_imageInfoUav{ descExt.imageInfoUav }
{ }


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