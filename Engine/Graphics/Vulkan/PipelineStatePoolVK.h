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
#include "Graphics\ResourcePool.h"
#include "Graphics\Vulkan\VulkanCommon.h"


namespace Luna
{

class Shader;

} // namespace Luna


namespace Luna::VK
{

struct PipelineStateData
{
	wil::com_ptr<CVkPipeline> pipeline;
};


class PipelineStateFactory
{
public:
	PipelineStateFactory() = default;
	~PipelineStateFactory();

	void SetDevice(CVkDevice* device);
	PipelineStateData Create(const GraphicsPipelineDesc& pipelineDesc);

private:
	wil::com_ptr<CVkShaderModule> CreateShaderModule(Shader* shader);
	wil::com_ptr<CVkPipelineCache> CreatePipelineCache();

private:
	wil::com_ptr<CVkDevice> m_device;

	// Pipelines and shader modules
	std::mutex m_shaderModuleMutex;
	std::map<size_t, wil::com_ptr<CVkShaderModule>> m_shaderModuleHashMap;
	wil::com_ptr<CVkPipelineCache> m_pipelineCache;
};


class PipelineStatePool 
	: public IPipelineStatePool
	, public ResourcePool1<PipelineStateFactory, GraphicsPipelineDesc, PipelineStateData, 4096>
{
	static const uint32_t MaxItems = (1 << 12);

public:
	explicit PipelineStatePool(CVkDevice* device);
	~PipelineStatePool();

	// Create/Destroy pipeline state
	PipelineStateHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& pipelineDesc) override;
	void DestroyHandle(PipelineStateHandleType* handle) override;

	// Platform agnostic getters
	const GraphicsPipelineDesc& GetDesc(PipelineStateHandleType* handle) const override;

	// Getters
	VkPipeline GetPipeline(PipelineStateHandleType* handle) const;

private:
	wil::com_ptr<CVkDevice> m_device;
};


PipelineStatePool* const GetVulkanPipelineStatePool();

} // namespace Luna::VK
