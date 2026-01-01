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

VkImage Descriptor::GetImage() const
{
	if (m_image)
	{
		return m_image->Get();
	}
	return VK_NULL_HANDLE;
}


VkBuffer Descriptor::GetBuffer() const
{
	if (m_buffer)
	{
		return m_buffer->Get();
	}
	return VK_NULL_HANDLE;
}


void Descriptor::SetImageView(CVkImage* image, CVkImageView* imageView)
{
	m_image = image;
	m_imageView = imageView;
	m_descriptorClass = DescriptorClass::Image;
}


void Descriptor::SetBufferView(CVkBuffer* buffer, CVkBufferView* bufferView, size_t elementSize, VkFormat format)
{
	m_buffer = buffer;
	m_bufferView = bufferView;
	m_elementSize = elementSize;
	m_format = format;
	m_descriptorClass = DescriptorClass::Buffer;
}


void Descriptor::SetSampler(CVkSampler* sampler)
{
	m_sampler = sampler;
	m_descriptorClass = DescriptorClass::Sampler;
}

} // namespace Luna::VK