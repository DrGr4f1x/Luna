//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#pragma once

#include "Graphics\Descriptor.h"
#include "Graphics\Vulkan\VulkanCommon.h"


namespace Luna::VK
{

// Forward declarations
class Device;


class Descriptor : public IDescriptor
{
public:
	~Descriptor() override {}

	DescriptorClass GetDescriptorClass() const noexcept { return m_descriptorClass; }

	VkImageView GetImageView() const { return m_imageView->Get(); }
	VkBufferView GetBufferView() const { return m_bufferView->Get(); }
	VkSampler GetSampler() const { return m_sampler->Get(); }

	VkImage GetImage() const;
	VkBuffer GetBuffer() const;
	size_t GetElementSize() const noexcept { return m_elementSize; }

	void SetImageView(CVkImage* image, CVkImageView* imageView);
	void SetBufferView(CVkBuffer* buffer, CVkBufferView* bufferView, size_t elementSize);
	void SetSampler(CVkSampler* sampler);
	
private:
	union 
	{
		wil::com_ptr<CVkImageView> m_imageView{};
		wil::com_ptr<CVkBufferView> m_bufferView;
		wil::com_ptr<CVkSampler> m_sampler;
	};

	wil::com_ptr<CVkImage> m_image;
	wil::com_ptr<CVkBuffer> m_buffer;
	size_t m_elementSize{ 0 };

	DescriptorClass m_descriptorClass{ DescriptorClass::None };
};

} // namespace Luna::VK