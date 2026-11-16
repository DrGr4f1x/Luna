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

#include "Graphics\RayTracing\ShaderBindingTable.h"

#include "Graphics\GpuBuffer.h"
#include "Graphics\Vulkan\VulkanCommon.h"

namespace Luna::VK
{

class ShaderBindingTable : public IShaderBindingTable
{
	friend class Device;

public:
	StridedAddressRegion GetRayGenerationShaderRecord() const override { return m_rayGenRegion; }
	StridedAddressRegion GetMissShaderTable() const override { return m_missRegion; }
	StridedAddressRegion GetHitGroupTable() const override { return m_hitGroupRegion; }
	StridedAddressRegion GetCallableShaderTable() const override { return m_callableRegion; }

protected:
	GpuBufferPtr m_sbt;

	StridedAddressRegion m_rayGenRegion{};
	StridedAddressRegion m_missRegion{};
	StridedAddressRegion m_hitGroupRegion{};
	StridedAddressRegion m_callableRegion{};
};

} // namespace Luna::VK