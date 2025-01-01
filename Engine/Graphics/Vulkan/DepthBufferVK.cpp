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

} // namespace Luna::VK