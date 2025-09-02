//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#include "Stdafx.h"

#include "PipelineStateUtilVK.h"

using namespace std;


namespace Luna::VK
{


void FillRasterizerState(VkPipelineRasterizationStateCreateInfo& createInfo, const RasterizerStateDesc& desc)
{
	createInfo = VkPipelineRasterizationStateCreateInfo{
		.sType						= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable			= VK_FALSE,
		.rasterizerDiscardEnable	= desc.rasterizerDiscardEnable ? VK_TRUE : VK_FALSE,
		.polygonMode				= FillModeToVulkan(desc.fillMode),
		.cullMode					= CullModeToVulkan(desc.cullMode),
		.frontFace					= desc.frontCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable			= (desc.depthBias != 0 || desc.slopeScaledDepthBias != 0.0f) ? VK_TRUE : VK_FALSE,
		.depthBiasConstantFactor	= desc.depthBias,
		.depthBiasClamp				= desc.depthBiasClamp,
		.depthBiasSlopeFactor		= desc.slopeScaledDepthBias,
		.lineWidth					= 1.0f
	};
}


void FillDepthStencilState(VkPipelineDepthStencilStateCreateInfo& createInfo, const DepthStencilStateDesc& desc)
{
	createInfo = VkPipelineDepthStencilStateCreateInfo{
		.sType					= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable		= desc.depthEnable ? VK_TRUE : VK_FALSE,
		.depthWriteEnable		= desc.depthWriteMask == DepthWrite::All ? VK_TRUE : VK_FALSE,
		.depthCompareOp			= ComparisonFuncToVulkan(desc.depthFunc),
		.depthBoundsTestEnable	= desc.depthBoundsTestEnable ? VK_TRUE : VK_FALSE,
		.stencilTestEnable		= desc.stencilEnable ? VK_TRUE : VK_FALSE,
		.front = {
			.failOp			= StencilOpToVulkan(desc.frontFace.stencilFailOp),
			.passOp			= StencilOpToVulkan(desc.frontFace.stencilPassOp),
			.depthFailOp	= StencilOpToVulkan(desc.frontFace.stencilDepthFailOp),
			.compareOp		= ComparisonFuncToVulkan(desc.frontFace.stencilFunc),
			.compareMask	= desc.frontFace.stencilReadMask,
			.writeMask		= desc.frontFace.stencilWriteMask,
			.reference		= 0
		},
		.back = {
			.failOp			= StencilOpToVulkan(desc.backFace.stencilFailOp),
			.passOp			= StencilOpToVulkan(desc.backFace.stencilPassOp),
			.depthFailOp	= StencilOpToVulkan(desc.backFace.stencilDepthFailOp),
			.compareOp		= ComparisonFuncToVulkan(desc.backFace.stencilFunc),
			.compareMask	= desc.backFace.stencilReadMask,
			.writeMask		= desc.backFace.stencilWriteMask,
			.reference		= 0
		},
		.minDepthBounds			= 0.0f,
		.maxDepthBounds			= 1.0f
	};
}


void FillDynamicStates(VkPipelineDynamicStateCreateInfo& createInfo, vector<VkDynamicState>& dynamicStates, bool isMeshletState)
{
	dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
	if (!isMeshletState)
	{
		dynamicStates.push_back(VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY);
	}
	dynamicStates.push_back(VK_DYNAMIC_STATE_STENCIL_REFERENCE);
	dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);

	createInfo = VkPipelineDynamicStateCreateInfo{
		.sType				= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount	= (uint32_t)dynamicStates.size(),
		.pDynamicStates		= dynamicStates.data()
	};
}


void FillViewportState(VkPipelineViewportStateCreateInfo& createInfo)
{
	createInfo = VkPipelineViewportStateCreateInfo{
		.sType			= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount	= 1,
		.pViewports		= nullptr, // dynamic state
		.scissorCount	= 1,
		.pScissors		= nullptr  // dynamic state
	};
}

} // namespace Luna::VK