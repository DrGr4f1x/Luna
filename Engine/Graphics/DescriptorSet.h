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

namespace Luna
{

// Forward declarations
class IColorBuffer;
class IDepthBuffer;
class IGpuBuffer;


class IDescriptorSet
{
public:
	virtual ~IDescriptorSet() = default;

	virtual void SetSRV(uint32_t slot, const IColorBuffer* colorBuffer) = 0;
	virtual void SetSRV(uint32_t slot, const IDepthBuffer* depthBuffer, bool depthSrv = true) = 0;
	virtual void SetSRV(uint32_t slot, const IGpuBuffer* gpuBuffer) = 0;

	virtual void SetUAV(uint32_t slot, const IColorBuffer* colorBuffer, uint32_t uavIndex = 0) = 0;
	virtual void SetUAV(uint32_t slot, const IDepthBuffer* depthBuffer) = 0;
	virtual void SetUAV(uint32_t slot, const IGpuBuffer* gpuBuffer) = 0;

	virtual void SetCBV(uint32_t slot, const IGpuBuffer* gpuBuffer) = 0;

	virtual void SetDynamicOffset(uint32_t offset) = 0;

	virtual void UpdateGpuDescriptors() = 0;
};

using DescriptorSetPtr = std::shared_ptr<IDescriptorSet>;

} // namespace Luna