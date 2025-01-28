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

#include "Graphics\PipelineState.h"
#include "Graphics\Vulkan\VulkanCommon.h"

using namespace Microsoft::WRL;


namespace Luna::VK
{

struct GraphicsPSODescExt
{
	VkPipeline pipeline{ VK_NULL_HANDLE };
};


class __declspec(uuid("90FD0A7C-E98E-4FA8-BE79-D0D54D87915A")) IGraphicsPSOData : public IPlatformData
{
public:
	virtual VkPipeline GetPipeline() const noexcept = 0;
};


class __declspec(uuid("48AF3A85-14E9-4820-A1F2-F08AF1157234")) GraphicsPSOData final
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IGraphicsPSOData, IPlatformData>>
	, NonCopyable
{
public:
	explicit GraphicsPSOData(const GraphicsPSODescExt& descExt);

	VkPipeline GetPipeline() const noexcept override { return m_pipeline; }

private:
	VkPipeline m_pipeline{ VK_NULL_HANDLE };
};

} // namespace Luna::VK