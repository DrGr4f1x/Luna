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

#include "DescriptorVK.h"


namespace Luna::VK
{

Descriptor::~Descriptor()
{}


void Descriptor::SetImageView(CVkImageView* imageView)
{
	m_imageView = imageView;
}


void Descriptor::SetBufferView(CVkBufferView* bufferView)
{
	m_bufferView = bufferView;
}


void Descriptor::SetSampler(CVkSampler* sampler)
{
	m_sampler = sampler;
}

} // namespace Luna::VK