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

VkImageView DepthBuffer::GetImageView(DepthStencilAspect depthStencilAspect) const
{ 
	switch (depthStencilAspect)
	{
	case DepthStencilAspect::DepthReadOnly:		return m_imageViewDepthOnly->Get();
	case DepthStencilAspect::StencilReadOnly:	return m_imageViewStencilOnly->Get();
	default:									return m_imageViewDepthStencil->Get();
	}
}

} // namespace Luna::VK