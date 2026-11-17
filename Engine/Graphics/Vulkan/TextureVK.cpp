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

#include "TextureVK.h"


namespace Luna::VK
{

Texture::Texture(const TextureDesc& desc, Device* device, CVkImage* image, const std::string& name)
	: ITexture{ desc }
	, m_device{ device }
	, m_image{ image }
	, m_name{ name }
{}

} // namespace Luna::VK