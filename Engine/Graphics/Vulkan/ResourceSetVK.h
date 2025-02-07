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


namespace Luna::VK
{

// Forward declarations
class IDescriptorSetVK;


class __declspec(uuid("E9C7A539-399F-4AB8-9E46-09636214C9CC")) IResourceSetVK : public IResourceSet
{
public:
	virtual IDescriptorSetVK* GetDescriptorSet(uint32_t index) = 0;
	virtual const IDescriptorSetVK* GetDescriptorSet(uint32_t index) const = 0;

	IDescriptorSetVK* operator[](uint32_t index) { return GetDescriptorSet(index); }
	const IDescriptorSetVK* operator[](uint32_t index) const { return GetDescriptorSet(index); }
};


class __declspec(uuid("7D7AA274-908C-4FD5-8A92-EE5DF1E77645")) ResourceSet
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IResourceSetVK, IResourceSet>>
	, NonCopyable
{
public:
	explicit ResourceSet(const std::array<wil::com_ptr<IDescriptorSet>, MaxRootParameters>& descriptorSets);

	// IDescriptorSet implementation
	void SetSRV(int param, int slot, const IColorBuffer* colorBuffer) override;
	void SetSRV(int param, int slot, const IDepthBuffer* depthBuffer, bool depthSrv = true) override;
	void SetSRV(int param, int slot, const IGpuBuffer* gpuBuffer) override;

	void SetUAV(int param, int slot, const IColorBuffer* colorBuffer, uint32_t uavIndex = 0) override;
	void SetUAV(int param, int slot, const IDepthBuffer* depthBuffer) override;
	void SetUAV(int param, int slot, const IGpuBuffer* gpuBuffer) override;

	void SetCBV(int param, int slot, const IGpuBuffer* gpuBuffer) override;

	void SetDynamicOffset(int param, uint32_t offset) override;

	// IDescriptorSetVK implementation
	IDescriptorSetVK* GetDescriptorSet(uint32_t index) override;
	const IDescriptorSetVK* GetDescriptorSet(uint32_t index) const override;

private:
	std::array<wil::com_ptr<IDescriptorSetVK>, MaxRootParameters> m_descriptorSets;
};

} // namespace Luna::VK