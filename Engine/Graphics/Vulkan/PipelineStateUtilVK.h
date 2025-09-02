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


namespace Luna::VK
{

template <class TPipelineStateDesc>
void FillMultisampleState(VkPipelineMultisampleStateCreateInfo& createInfo, const TPipelineStateDesc& desc)
{
	createInfo = VkPipelineMultisampleStateCreateInfo{
		.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples	= GetSampleCountFlags(desc.msaaCount),
		.sampleShadingEnable	= desc.sampleRateShading ? VK_TRUE : VK_FALSE,
		.minSampleShading		= desc.sampleRateShading ? 0.25f : 0.0f,
		.pSampleMask			= nullptr,
		.alphaToCoverageEnable	= desc.blendState.alphaToCoverageEnable ? VK_TRUE : VK_FALSE,
		.alphaToOneEnable		= VK_FALSE
	};
}

void FillRasterizerState(VkPipelineRasterizationStateCreateInfo& createInfo, const RasterizerStateDesc& desc);

void FillDepthStencilState(VkPipelineDepthStencilStateCreateInfo& createInfo, const DepthStencilStateDesc& desc);

template <class TPipelineStateDesc>
void FillBlendState(VkPipelineColorBlendStateCreateInfo& createInfo, std::array<VkPipelineColorBlendAttachmentState, 8>& blendAttachments, const TPipelineStateDesc& desc)
{
	const auto& blendState = desc.blendState;

	for (uint32_t i = 0; i < 8; ++i)
	{
		const auto& rt = blendState.renderTargetBlend[i];
		blendAttachments[i].blendEnable = rt.blendEnable ? VK_TRUE : VK_FALSE;
		blendAttachments[i].srcColorBlendFactor = BlendToVulkan(rt.srcBlend);
		blendAttachments[i].dstColorBlendFactor = BlendToVulkan(rt.dstBlend);
		blendAttachments[i].colorBlendOp = BlendOpToVulkan(rt.blendOp);
		blendAttachments[i].srcAlphaBlendFactor = BlendToVulkan(rt.srcBlendAlpha);
		blendAttachments[i].dstAlphaBlendFactor = BlendToVulkan(rt.dstBlendAlpha);
		blendAttachments[i].alphaBlendOp = BlendOpToVulkan(rt.blendOpAlpha);
		blendAttachments[i].colorWriteMask = ColorWriteToVulkan(rt.writeMask);

		// First render target with logic op enabled gets to set the state
		if (rt.logicOpEnable && (VK_FALSE == createInfo.logicOpEnable))
		{
			createInfo.logicOpEnable = VK_TRUE;
			createInfo.logicOp = LogicOpToVulkan(rt.logicOp);
		}
	}
	const uint32_t numRtvs = (uint32_t)desc.rtvFormats.size();
	createInfo.attachmentCount = numRtvs;
	createInfo.pAttachments = blendAttachments.data();
}


void FillDynamicStates(VkPipelineDynamicStateCreateInfo& createInfo, std::vector<VkDynamicState>& dynamicStates, bool isMeshletState);

template <class TPipelineStateDesc>
void FillRenderTargetState(VkPipelineRenderingCreateInfo& createInfo, std::vector<VkFormat>& rtvFormats, const TPipelineStateDesc& desc)
{
	const uint32_t numRtvs = (uint32_t)desc.rtvFormats.size();
	if (numRtvs > 0)
	{
		for (uint32_t i = 0; i < numRtvs; ++i)
		{
			rtvFormats[i] = FormatToVulkan(desc.rtvFormats[i]);
		}
	}

	const Format dsvFormat = desc.dsvFormat;
	createInfo = VkPipelineRenderingCreateInfo{
		.sType						= VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
		.viewMask					= 0,
		.colorAttachmentCount		= (uint32_t)rtvFormats.size(),
		.pColorAttachmentFormats	= rtvFormats.data(),
		.depthAttachmentFormat		= FormatToVulkan(dsvFormat),
		.stencilAttachmentFormat	= IsStencilFormat(dsvFormat) ? FormatToVulkan(dsvFormat) : VK_FORMAT_UNDEFINED
	};
}

void FillViewportState(VkPipelineViewportStateCreateInfo& createInfo);

} // namespace Luna::VK