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
	GpuBuffer(
		const GpuBufferDesc& desc,
		CVkBuffer* buffer,
		VkDeviceAddress deviceAddress,
		bool isCpuWriteable);

	GpuBuffer(
		const GpuBufferDesc& desc,
		CVkBuffer* buffer,
		CVkAccelerationStructure* accelerationStructure,
		VkDeviceAddress deviceAddress,
		VkDeviceAddress accelerationStructureDeviceAddress,
		bool isCpuWriteable);

	void Update(size_t sizeInBytes, const void* data) override;
	void Update(size_t sizeInBytes, size_t offset, const void* data) override;

	void* Map() override;
	void Unmap() override;

	const IDescriptor* GetSrvDescriptor() const noexcept override { return &m_srvDescriptor; }
	const IDescriptor* GetUavDescriptor() const noexcept override { return &m_uavDescriptor; }
	const IDescriptor* GetCbvDescriptor() const noexcept override { return &m_cbvDescriptor; }

	uint64_t GetGpuAddress() const noexcept override 
	{ 
		assert(m_deviceAddress != 0);
		return m_deviceAddress; 
	}

	uint64_t GetAccelerationStructureDeviceAddress() const noexcept 
	{ 
		assert(m_accelerationStructureDeviceAddress != 0);
		return m_accelerationStructureDeviceAddress;
	}

	VkBuffer GetBuffer() const noexcept;
	VkAccelerationStructureKHR GetAccelerationStructure() const noexcept;

	void InitializeDescriptors(Device* device, bool hasSrv, bool hasUav);

protected:
	wil::com_ptr<CVkBuffer> m_buffer;
	wil::com_ptr<CVkAccelerationStructure> m_accelerationStructure;
	VkDeviceAddress m_deviceAddress{ 0 };
	VkDeviceAddress m_accelerationStructureDeviceAddress{ 0 };
	bool m_isCpuWriteable{ false };

	Descriptor m_srvDescriptor;
	Descriptor m_uavDescriptor;
	Descriptor m_cbvDescriptor;
};

} // namespace Luna::VK