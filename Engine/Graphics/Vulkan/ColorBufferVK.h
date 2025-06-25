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

#include "Graphics\ColorBuffer.h"
#include "Graphics\Vulkan\VulkanCommon.h"


namespace Luna::VK
{

// Forward declarations
class Device;


class ColorBuffer : public IColorBuffer
{
	friend class Device;

public:
	VkImage GetImage() const { return m_image->Get(); }
	VkImageView GetImageViewRtv() const { return m_imageViewRtv->Get(); }
	VkImageView GetImageViewSrv() const { return m_imageViewSrv->Get(); }
	VkDescriptorImageInfo GetImageInfoSrv() const { return m_imageInfoSrv; }
	VkDescriptorImageInfo GetImageInfoUav() const { return m_imageInfoUav; }

protected:
	Device* m_device{ nullptr };
	wil::com_ptr<CVkImage> m_image;
	wil::com_ptr<CVkImageView> m_imageViewRtv;
	wil::com_ptr<CVkImageView> m_imageViewSrv;
	VkDescriptorImageInfo m_imageInfoSrv{};
	VkDescriptorImageInfo m_imageInfoUav{};
};

using ColorBufferPtr = std::shared_ptr<ColorBuffer>;

} // namespace Luna::VK