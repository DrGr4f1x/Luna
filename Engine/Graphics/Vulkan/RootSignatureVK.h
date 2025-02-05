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

#include "Graphics\RootSignature.h"
#include "Graphics\Vulkan\VulkanCommon.h"

using namespace Microsoft::WRL;


namespace Luna::VK
{

struct RootSignatureDescExt
{
	CVkPipelineLayout* pipelineLayout{ nullptr };
	std::vector<wil::com_ptr<CVkDescriptorSetLayout>> descriptorSetLayouts;

	constexpr RootSignatureDescExt& SetPipelineLayout(CVkPipelineLayout* value) noexcept { pipelineLayout = value; return *this; }
	RootSignatureDescExt& SetDescriptorSetLayouts(const std::vector<wil::com_ptr<CVkDescriptorSetLayout>>& value) { descriptorSetLayouts = value; return *this; }
};


class __declspec(uuid("F2CBB8B8-A5FA-4157-8CA0-3CD53B79EE93")) IRootSignatureVK : public IRootSignature
{
public:
	virtual VkPipelineLayout GetPipelineLayout() const noexcept = 0;
};


class __declspec(uuid("5E1B4285-CC94-4950-8668-BF74E0BE0FF4")) RootSignatureVK final
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IRootSignatureVK, IRootSignature>>
	, NonCopyable
{
public:
	RootSignatureVK(const RootSignatureDesc& rootSignatureDesc, const RootSignatureDescExt& rootSignatureDescExt);

	// IRootSignature implementation
	NativeObjectPtr GetNativeObject(NativeObjectType type) const noexcept override;

	uint32_t GetNumRootParameters() const noexcept override;
	const RootParameter& GetRootParameter(uint32_t index) const noexcept override;

	wil::com_ptr<IDescriptorSet> CreateDescriptorSet(uint32_t index) const override;

	// IRootSignatureVK implementation
	VkPipelineLayout GetPipelineLayout() const noexcept override { return m_pipelineLayout->Get(); }

private:
	const RootSignatureDesc m_desc;
	wil::com_ptr<CVkPipelineLayout> m_pipelineLayout;
	std::vector<wil::com_ptr<CVkDescriptorSetLayout>> m_descriptorSetLayouts;
};

} // namespace Luna::VK
