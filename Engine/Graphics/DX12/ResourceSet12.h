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

#include "Graphics\ResourceSet.h"

using namespace Microsoft::WRL;


namespace Luna::DX12
{

// Forward declarations
class IDescriptorSet12;


class __declspec(uuid("9DF7E866-B848-4F6B-838D-CF0E666231B4")) IResourceSet12 : public IResourceSet
{
public:
	virtual IDescriptorSet12* GetDescriptorSet(uint32_t index) = 0;
	virtual const IDescriptorSet12* GetDescriptorSet(uint32_t index) const = 0;

	IDescriptorSet12* operator[](uint32_t index) { return GetDescriptorSet(index); }
	const IDescriptorSet12* operator[](uint32_t index) const { return GetDescriptorSet(index); }
};


class __declspec(uuid("4CFAE961-6CF9-46BC-84DA-6E727A564F30")) ResourceSet
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IResourceSet12, IResourceSet>>
	, NonCopyable
{
public:
	explicit ResourceSet(const std::vector<wil::com_ptr<IDescriptorSet>>& descriptorSets);

	// IDescriptorSet implementation
	void SetSRV(int param, int slot, const IColorBuffer* colorBuffer) override;
	void SetSRV(int param, int slot, const IDepthBuffer* depthBuffer, bool depthSrv = true) override;
	void SetSRV(int param, int slot, const IGpuBuffer* gpuBuffer) override;

	void SetUAV(int param, int slot, const IColorBuffer* colorBuffer, uint32_t uavIndex = 0) override;
	void SetUAV(int param, int slot, const IDepthBuffer* depthBuffer) override;
	void SetUAV(int param, int slot, const IGpuBuffer* gpuBuffer) override;

	void SetCBV(int param, int slot, const IGpuBuffer* gpuBuffer) override;

	void SetDynamicOffset(int param, uint32_t offset) override;

	uint32_t GetNumDescriptorSets() const override { return (uint32_t)m_descriptorSets.size(); }

	// IDescriptorSet12 implementation
	IDescriptorSet12* GetDescriptorSet(uint32_t index) override;
	const IDescriptorSet12* GetDescriptorSet(uint32_t index) const override;

private:
	std::vector<wil::com_ptr<IDescriptorSet12>> m_descriptorSets;
};

} // namespace Luna::DX12