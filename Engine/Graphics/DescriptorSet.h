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

#include "Graphics\ResourceManager.h"


namespace Luna
{

// Forward declarations
class ColorBuffer;
class DepthBuffer;
class GpuBuffer;
class RootSignature;


class DescriptorSet
{
public:
	void Initialize(const RootSignature& rootSignature, uint32_t rootParamIndex);

	// TODO: change 'int slot' to uint32_t
	void SetSRV(int slot, const ColorBuffer& colorBuffer);
	void SetSRV(int slot, const DepthBuffer& depthBuffer, bool depthSrv = true);
	void SetSRV(int slot, const GpuBuffer& gpuBuffer);

	void SetUAV(int slot, const ColorBuffer& colorBuffer, uint32_t uavIndex = 0);
	void SetUAV(int slot, const DepthBuffer& depthBuffer);
	void SetUAV(int slot, const GpuBuffer& gpuBuffer);

	void SetCBV(int slot, const GpuBuffer& gpuBuffer);

	void SetDynamicOffset(uint32_t offset);

	ResourceHandle GetHandle() const { return m_handle; }

private:
	ResourceHandle m_handle;
};

} // namespace Luna