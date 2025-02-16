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
class ColorBuffer;
class DepthBuffer;
class GpuBuffer;
class RootSignature;


class ResourceSet
{
public:
	void Initialize(const RootSignature& rootSignature);

	// TODO: change 'int param' and 'int slot' to uint32_t
	void SetSRV(int param, int slot, const ColorBuffer& colorBuffer);
	void SetSRV(int param, int slot, const DepthBuffer& depthBuffer, bool depthSrv = true);
	void SetSRV(int param, int slot, const GpuBuffer& gpuBuffer);

	void SetUAV(int param, int slot, const ColorBuffer& colorBuffer, uint32_t uavIndex = 0);
	void SetUAV(int param, int slot, const DepthBuffer& depthBuffer);
	void SetUAV(int param, int slot, const GpuBuffer& gpuBuffer);

	void SetCBV(int param, int slot, const GpuBuffer& gpuBuffer);

	void SetDynamicOffset(int param, uint32_t offset);

	uint32_t GetNumDescriptorSets() const { return (uint32_t)m_descriptorSets.size(); }

	DescriptorSet& operator[](uint32_t index);
	const DescriptorSet& operator[](uint32_t index) const;

private:
	std::vector<DescriptorSet> m_descriptorSets;
};

} // namespace Luna