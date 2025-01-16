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


class __declspec(uuid("75D3C0C6-A72C-4F6F-94BF-8FDBF0647A62")) IRootSignatureData : public IPlatformData
{
public:
	virtual VkPipelineLayout GetPipelineLayout() const noexcept = 0;
};


class __declspec(uuid("2E54BC37-A9D2-4989-A071-78C3689D9112")) RootSignatureData final
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IRootSignatureData, IPlatformData>>
	, NonCopyable
{
public:
	explicit RootSignatureData(const RootSignatureDescExt& descExt);

	VkPipelineLayout GetPipelineLayout() const noexcept override { return m_pipelineLayout->Get(); }

private:
	wil::com_ptr<CVkPipelineLayout> m_pipelineLayout;
	std::vector<wil::com_ptr<CVkDescriptorSetLayout>> m_descriptorSetLayouts;
};

} // namespace Luna::VK
