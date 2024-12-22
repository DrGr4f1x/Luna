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

ColorBuffer::ColorBuffer(const ColorBufferDesc& desc, const ColorBufferDescExt& descExt)
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
	, m_clearColor{ desc.clearColor }
	, m_imageViewRtv{ descExt.imageViewRtv }
	, m_imageViewSrv{ descExt.imageViewSrv }
	, m_imageInfoSrv{ descExt.imageInfoSrv }
	, m_imageInfoUav{ descExt.imageInfoUav }
{
	m_numMips = m_numMips == 0 ? ComputeNumMips(m_width, m_height) : m_numMips;
}


NativeObjectPtr ColorBuffer::GetNativeObject(NativeObjectType nativeObjectType) const noexcept
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