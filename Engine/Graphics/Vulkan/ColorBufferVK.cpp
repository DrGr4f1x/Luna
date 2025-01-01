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

} // namespace Luna::VK