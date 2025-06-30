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

#include "Graphics\GraphicsCommon.h"
#include "Graphics\DescriptorSet.h"


namespace Luna
{

// Forward declarations
class IColorBuffer;
class IDepthBuffer;
class IGpuBuffer;
class IRootSignature;
class ISampler;
class TexturePtr;

using ColorBufferPtr = std::shared_ptr<IColorBuffer>;
using DepthBufferPtr = std::shared_ptr<IDepthBuffer>;
using GpuBufferPtr = std::shared_ptr<IGpuBuffer>;
using RootSignaturePtr = std::shared_ptr<IRootSignature>;
using SamplerPtr = std::shared_ptr<ISampler>;

class ResourceSet
{
public:
	void Initialize(RootSignaturePtr rootSignature);

	// TODO: change 'int param' and 'int slot' to uint32_t
	void SetSRV(int param, int slot, ColorBufferPtr colorBuffer);
	void SetSRV(int param, int slot, DepthBufferPtr depthBuffer, bool depthSrv = true);
	void SetSRV(int param, int slot, GpuBufferPtr gpuBuffer);
	void SetSRV(int param, int slot, TexturePtr texture);

	void SetUAV(int param, int slot, ColorBufferPtr colorBuffer, uint32_t uavIndex = 0);
	void SetUAV(int param, int slot, DepthBufferPtr depthBuffer);
	void SetUAV(int param, int slot, GpuBufferPtr gpuBuffer);

	void SetCBV(int param, int slot, GpuBufferPtr gpuBuffer);

	void SetSampler(int param, int slot, SamplerPtr sampler);

	void SetDynamicOffset(int param, uint32_t offset);

	uint32_t GetNumDescriptorSets() const { return (uint32_t)m_descriptorSets.size(); }

	DescriptorSetPtr& operator[](uint32_t index);
	const DescriptorSetPtr& operator[](uint32_t index) const;

private:
	std::vector<DescriptorSetPtr> m_descriptorSets;
};

} // namespace Luna