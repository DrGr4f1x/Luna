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
class IDescriptor;
class IGpuBuffer;
class ISampler;
class TexturePtr;

using ColorBufferPtr = std::shared_ptr<IColorBuffer>;
using DepthBufferPtr = std::shared_ptr<IDepthBuffer>;
using GpuBufferPtr = std::shared_ptr<IGpuBuffer>;
using SamplerPtr = std::shared_ptr<ISampler>;


class IDescriptorSet
{
public:
	virtual ~IDescriptorSet() = default;

	virtual void SetSRV(uint32_t slot, const IDescriptor* descriptor) = 0;
	virtual void SetUAV(uint32_t slot, const IDescriptor* descriptor) = 0;
	virtual void SetCBV(uint32_t slot, const IDescriptor* descriptor) = 0;
	virtual void SetSampler(uint32_t slot, const IDescriptor* descriptor) = 0;

	virtual void SetSRV(uint32_t slot, ColorBufferPtr colorBuffer) = 0;
	virtual void SetSRV(uint32_t slot, DepthBufferPtr depthBuffer, bool depthSrv = true) = 0;
	virtual void SetSRV(uint32_t slot, GpuBufferPtr gpuBuffer) = 0;
	virtual void SetSRV(uint32_t slot, TexturePtr texture) = 0;

	virtual void SetUAV(uint32_t slot, ColorBufferPtr colorBuffer, uint32_t uavIndex = 0) = 0;
	virtual void SetUAV(uint32_t slot, DepthBufferPtr depthBuffer) = 0;
	virtual void SetUAV(uint32_t slot, GpuBufferPtr gpuBuffer) = 0;

	virtual void SetCBV(uint32_t slot, GpuBufferPtr gpuBuffer) = 0;

	virtual void SetSampler(uint32_t slot, SamplerPtr sampler) = 0;

	virtual void SetDynamicOffset(uint32_t offset) = 0;
};

using DescriptorSetPtr = std::shared_ptr<IDescriptorSet>;

} // namespace Luna