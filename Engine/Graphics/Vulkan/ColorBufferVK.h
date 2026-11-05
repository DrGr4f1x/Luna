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
#include "Graphics\Vulkan\DescriptorVK.h"
#include "Graphics\Vulkan\VulkanCommon.h"


namespace Luna::VK
{

// Forward declarations
class Device;


class ColorBuffer : public IColorBuffer
{
	friend class Device;

public:
	const IDescriptor* GetSrvDescriptor() const noexcept override { return &m_srvDescriptor; }
	const IDescriptor* GetRtvDescriptor() const noexcept override { assert(false);  return nullptr; }
	const IDescriptor* GetUavDescriptor(uint32_t index) const noexcept override;

	VkImageView GetRtvImageView() const { return m_rtvImageView->Get(); }
	void SetRtvImageView(CVkImageView* rtvImageView) { m_rtvImageView = rtvImageView; }

	VkImageView GetSrvImageView() const { return m_srvImageView->Get(); }
	void SetSrvImageView(CVkImageView* srvImageView) { m_srvImageView = srvImageView; }

	VkImage GetImage() const noexcept { return m_image->Get(); }

	void InitializeDescriptors(Device* device, bool initUavDescriptor);

protected:
	wil::com_ptr<CVkImage> m_image;
	wil::com_ptr<CVkImageView> m_rtvImageView;
	wil::com_ptr<CVkImageView> m_srvImageView;
	Descriptor m_srvDescriptor;
	Descriptor m_uavDescriptor;
};

} // namespace Luna::VK