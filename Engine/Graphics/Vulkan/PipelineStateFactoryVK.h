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


namespace Luna
{

// Forward declarations
class IResourceManager;
class Shader;

} // namespace Luna


namespace Luna::VK
{

class PipelineStateFactory : public PipelineStateFactoryBase
{
public:
	PipelineStateFactory(IResourceManager* owner, CVkDevice* device);

	ResourceHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& pipelineDesc);
	void Destroy(uint32_t index);

	const GraphicsPipelineDesc& GetDesc(uint32_t index) const;

	VkPipeline GetGraphicsPipeline(uint32_t index) const;

private:
	wil::com_ptr<CVkShaderModule> CreateShaderModule(Shader* shader);
	wil::com_ptr<CVkPipelineCache> CreatePipelineCache() const;

	void ResetGraphicsPipelineState(uint32_t index)
	{
		m_graphicsPipelineStates[index].reset();
	}

private:
	IResourceManager* m_owner{ nullptr };
	wil::com_ptr<CVkDevice> m_device;

	std::mutex m_mutex;

	std::queue<uint32_t> m_freeList;

	std::array<wil::com_ptr<CVkPipeline>, MaxResources> m_graphicsPipelineStates;

	// Shader modules
	std::mutex m_shaderModuleMutex;
	std::map<size_t, wil::com_ptr<CVkShaderModule>> m_shaderModuleHashMap;

	// Pipeline cache
	wil::com_ptr<CVkPipelineCache> m_pipelineCache;
};

} // namespace Luna::VK