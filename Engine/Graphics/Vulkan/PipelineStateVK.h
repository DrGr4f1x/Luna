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

class __declspec(uuid("88314541-D852-49A8-A249-2F4F9D76BC49")) GraphicsPipeline final
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, IGraphicsPipeline>
	, NonCopyable
{
public:
	explicit GraphicsPipeline(const GraphicsPipelineDesc& graphicsPipelineDesc, CVkPipeline* pipeline);

	NativeObjectPtr GetNativeObject() const noexcept override;

	PrimitiveTopology GetPrimitiveTopology() const noexcept override { return m_desc.topology; }

private:
	const GraphicsPipelineDesc m_desc;
	wil::com_ptr<CVkPipeline> m_pipeline;
};

} // namespace Luna::VK