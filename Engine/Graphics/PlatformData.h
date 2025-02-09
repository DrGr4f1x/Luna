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

#include "Graphics\DX12\DirectXCommon.h"
#include "Graphics\Vulkan\VulkanCommon.h"

namespace Luna
{

class GraphicsPSOData
{
public:
	explicit GraphicsPSOData(ID3D12PipelineState* pipelineState)
		: m_data{ pipelineState }
	{}

	explicit GraphicsPSOData(VK::CVkPipeline* pipeline)
		: m_data{ pipeline }
	{}

	ID3D12PipelineState* GetDX12() { return std::get<0>(m_data).get(); }
	VK::CVkPipeline* GetVulkan() { return std::get<1>(m_data).get(); }

private:
	std::variant<wil::com_ptr<ID3D12PipelineState>, wil::com_ptr<VK::CVkPipeline>> m_data;
};


class CpuDescriptorData
{
public:

private:

};

} // namespace Luna