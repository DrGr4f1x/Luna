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


namespace Luna
{

// Forward declarations
class IColorBuffer;
class IDepthBuffer;
class IDescriptorSet;
class IGpuBuffer;


class __declspec(uuid("DFF7ED90-141E-40FB-93A1-6A491DF7F028")) IResourceSet : public IUnknown
{
public:
	virtual void SetSRV(int param, int slot, const IColorBuffer * colorBuffer) = 0;
	virtual void SetSRV(int param, int slot, const IDepthBuffer * depthBuffer, bool depthSrv = true) = 0;
	virtual void SetSRV(int param, int slot, const IGpuBuffer * gpuBuffer) = 0;

	virtual void SetUAV(int param, int slot, const IColorBuffer * colorBuffer, uint32_t uavIndex = 0) = 0;
	virtual void SetUAV(int param, int slot, const IDepthBuffer * depthBuffer) = 0;
	virtual void SetUAV(int param, int slot, const IGpuBuffer * gpuBuffer) = 0;

	virtual void SetCBV(int param, int slot, const IGpuBuffer * gpuBuffer) = 0;

	virtual void SetDynamicOffset(int param, uint32_t offset) = 0;

	virtual uint32_t GetNumDescriptorSets() const = 0;
};

using ResourceSetHandle = wil::com_ptr<IResourceSet>;

} // namespace Luna