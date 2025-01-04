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
#include "Graphics\Vulkan\VulkanCommon.h"

namespace Luna::VK
{

struct GpuBufferDescExt
{
	CVkBuffer* buffer{ nullptr };
	VkDescriptorBufferInfo bufferInfo{};

	constexpr GpuBufferDescExt& SetBuffer(CVkBuffer* value) noexcept { buffer = value; return *this; }
	constexpr GpuBufferDescExt& SetBufferInfo(VkDescriptorBufferInfo value) noexcept { bufferInfo = value; return *this; }
};


class __declspec(uuid("D3865121-E191-4392-B102-F42B5195F06A")) IGpuBufferData : public IPlatformData
{
public:
	virtual VkBuffer GetBuffer() const noexcept = 0;
	virtual VkDescriptorBufferInfo GetDescriptorBufferInfo() const noexcept = 0;
};


class __declspec(uuid("E2C59E07-6826-4A63-9F79-E335F94DE4B5")) GpuBufferData final
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IGpuBufferData, IPlatformData>>
{
public:
	explicit GpuBufferData(const GpuBufferDescExt& descExt)
		: m_buffer{ descExt.buffer }
		, m_bufferInfo{ descExt.bufferInfo }
	{}

	VkBuffer GetBuffer() const noexcept override { return m_buffer->Get(); }
	VkDescriptorBufferInfo GetDescriptorBufferInfo() const noexcept override { return m_bufferInfo; }

protected:
	wil::com_ptr<CVkBuffer> m_buffer;
	VkDescriptorBufferInfo m_bufferInfo;
};


} // namespace Luna::VK