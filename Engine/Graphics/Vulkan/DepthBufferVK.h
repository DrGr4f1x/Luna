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
#include "Graphics\DepthBuffer.h"
#include "Graphics\Vulkan\VulkanCommon.h"

using namespace Microsoft::WRL;


namespace Luna::VK
{

struct DepthBufferDescExt
{
	CVkImage* image{ nullptr };
	CVkImageView* imageViewDepthStencil{ nullptr };
	CVkImageView* imageViewDepthOnly{ nullptr };
	CVkImageView* imageViewStencilOnly{ nullptr };
	VkDescriptorImageInfo imageInfoDepth{};
	VkDescriptorImageInfo imageInfoStencil{};
	ResourceState usageState{ ResourceState::Undefined };

	DepthBufferDescExt& SetImage(CVkImage* value) noexcept { image = value; return *this; }
	DepthBufferDescExt& SetImageViewDepthStencil(CVkImageView* value) noexcept { imageViewDepthStencil = value; return *this; }
	DepthBufferDescExt& SetImageViewDepthOnly(CVkImageView* value) noexcept { imageViewDepthOnly = value; return *this; }
	DepthBufferDescExt& SetImageViewStencilOnly(CVkImageView* value) noexcept { imageViewStencilOnly = value; return *this; }
	DepthBufferDescExt& SetImageInfoDepth(const VkDescriptorImageInfo& value) noexcept { imageInfoDepth = value; return *this; }
	DepthBufferDescExt& SetImageInfoStencil(const VkDescriptorImageInfo& value) noexcept { imageInfoStencil = value; return *this; }
	DepthBufferDescExt& SetUsageState(const ResourceState value) noexcept { usageState = value; return *this; }
};


class __declspec(uuid("88609DFB-84C1-44D4-88DC-96B5CBB07556")) IDepthBufferData : public IPlatformData
{
public:
	// Get pre-created CPU-visible descriptor handles
	virtual VkImage GetImage() const noexcept = 0;
	virtual VkImageView GetDepthStencilImageView() const noexcept = 0;
	virtual VkImageView GetDepthOnlyImageView() const noexcept = 0;
	virtual VkImageView GetStencilOnlyImageView() const noexcept = 0;
	virtual VkDescriptorImageInfo GetDepthImageInfo() const noexcept = 0;
	virtual VkDescriptorImageInfo GetStencilImageInfo() const noexcept = 0;
};


class __declspec(uuid("7F056BBE-DB42-4BA1-BCBB-A4AADF7669CC")) DepthBufferData
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IDepthBufferData, IPlatformData>>
	, NonCopyable
{
public:
	DepthBufferData(const DepthBufferDescExt& descExt) noexcept;

	VkImage GetImage() const noexcept final { return *m_image; }

	VkImageView GetDepthStencilImageView() const noexcept final { return *m_imageViewDepthStencil; }
	VkImageView GetDepthOnlyImageView() const noexcept final { return *m_imageViewDepthOnly; }
	VkImageView GetStencilOnlyImageView() const noexcept final { return *m_imageViewStencilOnly; }
	VkDescriptorImageInfo GetDepthImageInfo() const noexcept final { return m_imageInfoDepth; }
	VkDescriptorImageInfo GetStencilImageInfo() const noexcept final { return m_imageInfoStencil; }

private:
	wil::com_ptr<CVkImage> m_image;
	wil::com_ptr<CVkImageView> m_imageViewDepthStencil;
	wil::com_ptr<CVkImageView> m_imageViewDepthOnly;
	wil::com_ptr<CVkImageView> m_imageViewStencilOnly;
	VkDescriptorImageInfo m_imageInfoDepth{};
	VkDescriptorImageInfo m_imageInfoStencil{};
};

} // namespace Luna::VK