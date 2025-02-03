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


class __declspec(uuid("B744F600-1AE2-4B5C-961D-D122127F972F")) IDescriptorSet : public IUnknown
{
public:
	virtual void SetSRV(int paramIndex, const IColorBuffer* colorBuffer) = 0;
	virtual void SetSRV(int paramIndex, const IDepthBuffer* depthBuffer, bool depthSrv = true) = 0;
	virtual void SetSRV(int paramIndex, const IGpuBuffer* gpuBuffer) = 0;

	virtual void SetUAV(int paramIndex, const IColorBuffer* colorBuffer, uint32_t uavIndex = 0) = 0;
	virtual void SetUAV(int paramIndex, const IDepthBuffer* depthBuffer) = 0;
	virtual void SetUAV(int paramIndex, const IGpuBuffer* gpuBuffer) = 0;

	virtual void SetCBV(int paramIndex, const IGpuBuffer* gpuBuffer) = 0;

	virtual void SetDynamicOffset(uint32_t offset) = 0;
};

} // namespace Luna