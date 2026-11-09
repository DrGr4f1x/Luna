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

#include "Graphics\Vulkan\VulkanCommon.h"


namespace Luna::VK
{

struct BindingInfo
{
	VkDeviceSize offset{ 0 };
	VkDescriptorType type{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER };
};


class DescriptorBindingTemplate
{
	friend class Device;

public:
	BindingInfo GetBindingInfo(uint32_t bindingIndex) const noexcept;

private:
	std::unordered_map<uint32_t, BindingInfo> m_bindingInfoMap;
};


class DescriptorSetLayout
{
	friend class Device;

public:

	wil::com_ptr<CVkDescriptorSetLayout> GetDescriptorSetLayout() const noexcept;

	size_t GetHashCode() const noexcept { return m_hashcode; }
	size_t GetDescriptorSetSize() const noexcept { return m_layoutSize; }

	VkDeviceSize GetBindingOffset(uint32_t bindingIndex) const;

protected:
	Device* m_device{ nullptr };
	wil::com_ptr<CVkDescriptorSetLayout> m_descriptorSetLayout;
	size_t m_hashcode{ 0 };

	// Descriptor buffer data
	VkDeviceSize m_layoutSize{ 0 };
	std::shared_ptr<DescriptorBindingTemplate> m_bindingTemplate;
};

using DescriptorSetLayoutPtr = std::shared_ptr<DescriptorSetLayout>;


class IHeapDescriptorBindingMap
{
public:
	virtual uint32_t GetRegisterOffsetSRV(uint32_t srvRegister) const = 0;
	virtual uint32_t GetRegisterOffsetUAV(uint32_t uavRegister) const = 0;
	virtual uint32_t GetRegisterOffsetCBV(uint32_t cbvRegister) const = 0;
	virtual uint32_t GetRegisterOffsetSampler(uint32_t samplerRegister) const = 0;

	virtual void SetRegisterOffsetSRV(uint32_t srvRegister, uint32_t offset) = 0;
	virtual void SetRegisterOffsetUAV(uint32_t uavRegister, uint32_t offset) = 0;
	virtual void SetRegisterOffsetCBV(uint32_t cbvRegister, uint32_t offset) = 0;
	virtual void SetRegisterOffsetSampler(uint32_t samplerRegister, uint32_t offset) = 0;
};

using HeapDescriptorBindingMapPtr = std::shared_ptr<IHeapDescriptorBindingMap>;


class HeapDescriptorBindingMapCompact : public IHeapDescriptorBindingMap
{
	friend class Device;

public:
	HeapDescriptorBindingMapCompact();

	uint32_t GetRegisterOffsetSRV(uint32_t srvRegister) const override;
	uint32_t GetRegisterOffsetUAV(uint32_t uavRegister) const override;
	uint32_t GetRegisterOffsetCBV(uint32_t cbvRegister) const override;
	uint32_t GetRegisterOffsetSampler(uint32_t samplerRegister) const override;

	void SetRegisterOffsetSRV(uint32_t srvRegister, uint32_t offset) override;
	void SetRegisterOffsetUAV(uint32_t uavRegister, uint32_t offset) override;
	void SetRegisterOffsetCBV(uint32_t cbvRegister, uint32_t offset) override;
	void SetRegisterOffsetSampler(uint32_t samplerRegister, uint32_t offset) override;

private:
	std::array<uint32_t, 32> m_srvRegisterOffsetMap{};
	std::array<uint32_t, 32> m_uavRegisterOffsetMap{};
	std::array<uint32_t, 32> m_cbvRegisterOffsetMap{};
	std::array<uint32_t, 32> m_samplerRegisterOffsetMap{};
};


class HeapDescriptorBindingMapGeneral : public IHeapDescriptorBindingMap
{
	friend class Device;

public:
	uint32_t GetRegisterOffsetSRV(uint32_t srvRegister) const override;
	uint32_t GetRegisterOffsetUAV(uint32_t uavRegister) const override;
	uint32_t GetRegisterOffsetCBV(uint32_t cbvRegister) const override;
	uint32_t GetRegisterOffsetSampler(uint32_t samplerRegister) const override;

	void SetRegisterOffsetSRV(uint32_t srvRegister, uint32_t offset) override;
	void SetRegisterOffsetUAV(uint32_t uavRegister, uint32_t offset) override;
	void SetRegisterOffsetCBV(uint32_t cbvRegister, uint32_t offset) override;
	void SetRegisterOffsetSampler(uint32_t samplerRegister, uint32_t offset) override;

private:
	std::unordered_map<uint16_t, uint32_t> m_srvRegisterOffsetMap;
	std::unordered_map<uint16_t, uint32_t> m_uavRegisterOffsetMap;
	std::unordered_map<uint16_t, uint32_t> m_cbvRegisterOffsetMap;
	std::unordered_map<uint16_t, uint32_t> m_samplerRegisterOffsetMap;
};

} // namespace Luna::VK