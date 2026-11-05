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
#include "Graphics\Vulkan\DescriptorVK.h"
#include "Graphics\Vulkan\VulkanCommon.h"

namespace Luna::VK
{

// Forward declarations
class Device;


class DepthBuffer : public IDepthBuffer
{
	friend class Device;

public:
	const IDescriptor* GetDsvDescriptor(DepthStencilAspect aspect) const noexcept override { return nullptr; }
	const IDescriptor* GetSrvDescriptor(bool depthSrv) const noexcept override;

	void SetDepthStencilImageView(CVkImageView* imageView)
	{
		m_depthStencilImageView = imageView;
	}

	void SetDepthOnlyImageView(CVkImageView* imageView)
	{
		m_depthOnlyImageView = imageView;
	}

	void SetStencilOnlyImageView(CVkImageView* imageView)
	{
		m_stencilOnlyImageView = imageView;
	}

	VkImageView GetDsvImageView(DepthStencilAspect aspect) const;

	VkImage GetImage() const noexcept { return m_image->Get(); }

	void InitializeDescriptors(Device* device);

protected:
	wil::com_ptr<CVkImage> m_image;
	Descriptor m_depthStencilDescriptor;
	Descriptor m_depthOnlyDescriptor;
	Descriptor m_stencilOnlyDescriptor;

	wil::com_ptr<CVkImageView> m_depthStencilImageView;
	wil::com_ptr<CVkImageView> m_depthOnlyImageView;
	wil::com_ptr<CVkImageView> m_stencilOnlyImageView;
};

} // namespace Luna::VK