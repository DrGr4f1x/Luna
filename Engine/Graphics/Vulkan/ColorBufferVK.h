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

#include "Core\Color.h"
#include "Graphics\ColorBuffer.h"
#include "Graphics\Vulkan\VulkanCommon.h"

using namespace Microsoft::WRL;


namespace Luna::VK
{

struct ColorBufferDescExt
{
	CVkImage* image{ nullptr };
	CVkImageView* imageViewRtv{ nullptr };
	CVkImageView* imageViewSrv{ nullptr };
	VkDescriptorImageInfo imageInfoSrv{};
	VkDescriptorImageInfo imageInfoUav{};
	ResourceState usageState{ ResourceState::Undefined };

	ColorBufferDescExt& SetImage(CVkImage* value) noexcept { image = value; return *this; }
	ColorBufferDescExt& SetImageViewRtv(CVkImageView* value) noexcept { imageViewRtv = value; return *this; }
	ColorBufferDescExt& SetImageViewSrv(CVkImageView* value) noexcept { imageViewSrv = value; return *this; }
	ColorBufferDescExt& SetImageInfoSrv(const VkDescriptorImageInfo& value) noexcept { imageInfoSrv = value; return *this; }
	ColorBufferDescExt& SetImageInfoUav(const VkDescriptorImageInfo& value) noexcept { imageInfoUav = value; return *this; }
	ColorBufferDescExt& SetUsageState(const ResourceState value) noexcept { usageState = value; return *this; }
};


class __declspec(uuid("63AC7681-AE34-4F5B-9D96-DBEA9FF89CAB")) IColorBufferData : public IPlatformData
{
public:
	virtual VkImage GetImage() const noexcept = 0;
	virtual VkImageView GetImageViewRtv() const noexcept = 0;
	virtual VkImageView GetImageViewSrv() const noexcept = 0;
	virtual VkDescriptorImageInfo GetImageInfoSrv() const noexcept = 0;
	virtual VkDescriptorImageInfo GetImageInfoUav() const noexcept = 0;
};

class __declspec(uuid("23320A65-2603-49E9-B92F-7E5CE2A85BB3")) ColorBufferData
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IColorBufferData, IPlatformData>>
	, NonCopyable
{
public:
	explicit ColorBufferData(const ColorBufferDescExt& descExt);

	VkImage GetImage() const noexcept { return *m_image; }
	VkImageView GetImageViewRtv() const noexcept { return *m_imageViewRtv; }
	VkImageView GetImageViewSrv() const noexcept { return *m_imageViewSrv; }
	VkDescriptorImageInfo GetImageInfoSrv() const noexcept { return m_imageInfoSrv; }
	VkDescriptorImageInfo GetImageInfoUav() const noexcept { return m_imageInfoUav; }

private:
	wil::com_ptr<CVkImage> m_image;
	wil::com_ptr<CVkImageView> m_imageViewRtv;
	wil::com_ptr<CVkImageView> m_imageViewSrv;
	VkDescriptorImageInfo m_imageInfoSrv{};
	VkDescriptorImageInfo m_imageInfoUav{};
};

} // namespace Luna::VK