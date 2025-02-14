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
class IDescriptorSet;
class IGpuBuffer;
class RootSignature;


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


class ResourceSet
{
public:
	void Initialize(const RootSignature& rootSignature);

	// TODO: change 'int param' and 'int slot' to uint32_t
	void SetSRV(int param, int slot, const IColorBuffer* colorBuffer);
	void SetSRV(int param, int slot, const IDepthBuffer* depthBuffer, bool depthSrv = true);
	void SetSRV(int param, int slot, const IGpuBuffer* gpuBuffer);

	void SetUAV(int param, int slot, const IColorBuffer* colorBuffer, uint32_t uavIndex = 0);
	void SetUAV(int param, int slot, const IDepthBuffer* depthBuffer);
	void SetUAV(int param, int slot, const IGpuBuffer* gpuBuffer);

	void SetCBV(int param, int slot, const IGpuBuffer* gpuBuffer);

	void SetDynamicOffset(int param, uint32_t offset);

	uint32_t GetNumDescriptorSets() const { return (uint32_t)m_descriptorSets.size(); }

	DescriptorSet& operator[](uint32_t index);
	const DescriptorSet& operator[](uint32_t index) const;

private:
	std::vector<DescriptorSet> m_descriptorSets;
};

} // namespace Luna