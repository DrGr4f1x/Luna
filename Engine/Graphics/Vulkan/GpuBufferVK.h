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

#include "Graphics\GpuBuffer.h"
#include "Graphics\Vulkan\DescriptorVK.h"
#include "Graphics\Vulkan\VulkanCommon.h"


namespace Luna::VK
{

// Forward declarations
class Device;


class GpuBuffer : public IGpuBuffer
{
	friend class Device;

public:
	void Update(size_t sizeInBytes, const void* data) override;
	void Update(size_t sizeInBytes, size_t offset, const void* data) override;

	void* Map() override;
	void Unmap() override;

	VkBuffer GetBuffer() const noexcept;
	const Descriptor& GetDescriptor() const noexcept { return m_descriptor; }

protected:
	wil::com_ptr<CVkBuffer> m_buffer;
	Descriptor m_descriptor;

	bool m_isCpuWriteable{ false };
};

} // namespace Luna::VK