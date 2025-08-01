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

#include "Graphics\DepthBuffer.h"
#include "Graphics\Vulkan\VulkanCommon.h"

namespace Luna::VK
{

// Forward declarations
class Device;


class DepthBuffer : public IDepthBuffer
{
	friend class Device;

public:
	VkImage GetImage() const noexcept { return m_image->Get(); }
	VkImageView GetImageViewDepthStencil() const noexcept { return m_imageViewDepthStencil->Get(); }
	VkImageView GetImageViewDepthOnly() const noexcept { return m_imageViewDepthOnly->Get(); }
	VkImageView GetImageViewStencilOnly() const noexcept { return m_imageViewStencilOnly->Get(); }
	VkImageView GetImageView(DepthStencilAspect depthStencilAspect) const noexcept;
	VkDescriptorImageInfo GetImageInfoDepth() const noexcept { return m_imageInfoDepth; }
	VkDescriptorImageInfo GetImageInfoStencil() const noexcept { return m_imageInfoStencil; }

protected:
	Device* m_device{ nullptr };
	wil::com_ptr<CVkImage> m_image;
	wil::com_ptr<CVkImageView> m_imageViewDepthStencil;
	wil::com_ptr<CVkImageView> m_imageViewDepthOnly;
	wil::com_ptr<CVkImageView> m_imageViewStencilOnly;
	VkDescriptorImageInfo m_imageInfoDepth{};
	VkDescriptorImageInfo m_imageInfoStencil{};
};

} // namespace Luna::VK