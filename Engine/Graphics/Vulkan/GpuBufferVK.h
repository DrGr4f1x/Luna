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
	ResourceState usageState{ ResourceState::Undefined };

	constexpr GpuBufferDescExt& SetBuffer(CVkBuffer* value) noexcept { buffer = value; return *this; }
	constexpr GpuBufferDescExt& SetBufferInfo(VkDescriptorBufferInfo value) noexcept { bufferInfo = value; return *this; }
	constexpr GpuBufferDescExt& SetUsageState(ResourceState value) noexcept { usageState = value; return *this; }
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


class __declspec(uuid("12C950F5-CA09-4B2F-9362-B8369B6D4255")) IGpuBufferVK : public IGpuBuffer
{
public:
	virtual VkBuffer GetBuffer() const noexcept = 0;
	virtual VkDescriptorBufferInfo GetDescriptorBufferInfo() const noexcept = 0;
};


class __declspec(uuid("FBD3B7A5-D26B-4FDE-AF61-76DECDE904D3")) GpuBufferVK final
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IGpuBufferVK, IGpuBuffer, IGpuResource>>
	, NonCopyable
{
public:
	GpuBufferVK(const GpuBufferDesc& gpuBufferDesc, const GpuBufferDescExt& gpuBufferDescExt);

	// IGpuResource implementation
	const std::string& GetName() const override { return m_name; }
	ResourceType GetResourceType() const noexcept override { return m_resourceType; }

	ResourceState GetUsageState() const noexcept override { return m_usageState; }
	void SetUsageState(ResourceState usageState) noexcept override { m_usageState = usageState; }
	ResourceState GetTransitioningState() const noexcept override { return m_transitioningState; }
	void SetTransitioningState(ResourceState transitioningState) noexcept override { m_transitioningState = transitioningState; }

	NativeObjectPtr GetNativeObject(NativeObjectType type, uint32_t index = 0) const noexcept override;

	// IGpuBuffer implementation
	size_t GetSize() const noexcept override { return m_elementCount * m_elementSize; }
	size_t GetElementCount() const noexcept override { return m_elementCount; }
	size_t GetElementSize() const noexcept override { return m_elementSize; }

	// IGpuBufferVK implementation
	VkBuffer GetBuffer() const noexcept override { return m_buffer->Get(); }
	VkDescriptorBufferInfo GetDescriptorBufferInfo() const noexcept override { return m_bufferInfo; }

private:
	std::string m_name;
	ResourceType m_resourceType{ ResourceType::Texture2D };

	ResourceState m_usageState{ ResourceState::Undefined };
	ResourceState m_transitioningState{ ResourceState::Undefined };

	size_t m_elementCount{ 0 };
	size_t m_elementSize{ 0 };

	wil::com_ptr<CVkBuffer> m_buffer;
	VkDescriptorBufferInfo m_bufferInfo;
};

} // namespace Luna::VK