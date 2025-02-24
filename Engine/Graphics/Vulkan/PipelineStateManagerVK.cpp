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

#include "PipelineStateManagerVK.h"

#include "FileSystem.h"

#include "Graphics\Shader.h"

#include "DeviceManagerVK.h"
#include "RootSignaturePoolVK.h"

using namespace std;


namespace Luna::VK
{

PipelineStateManager* g_pipelineStateManager{ nullptr };


pair<string, bool> GetShaderFilenameWithExtension(const string& shaderFilename)
{
	auto fileSystem = GetFileSystem();

	string shaderFileWithExtension = shaderFilename;
	bool exists = false;

	// See if the filename already has an extension
	string extension = fileSystem->GetFileExtension(shaderFilename);
	if (!extension.empty())
	{
		exists = fileSystem->Exists(shaderFileWithExtension);
	}
	else
	{
		// Try .spirv extension
		shaderFileWithExtension = shaderFilename + ".spirv";
		exists = fileSystem->Exists(shaderFileWithExtension);
	}

	return make_pair(shaderFileWithExtension, exists);
}


Shader* LoadShader(ShaderType type, const ShaderNameAndEntry& shaderNameAndEntry)
{
	auto [shaderFilenameWithExtension, exists] = GetShaderFilenameWithExtension(shaderNameAndEntry.shaderFile);

	if (!exists)
	{
		return nullptr;
	}

	ShaderDesc shaderDesc{
		.filenameWithExtension = shaderFilenameWithExtension,
		.entry = shaderNameAndEntry.entry,
		.type = type
	};

	return Shader::Load(shaderDesc);
}

void FillShaderStageCreateInfo(VkPipelineShaderStageCreateInfo& createInfo, VkShaderModule shaderModule, const Shader* shader)
{
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.stage = ShaderTypeToVulkan(shader->GetShaderType());
	createInfo.pName = shader->GetEntry().c_str();
	createInfo.module = shaderModule;
	createInfo.pSpecializationInfo = nullptr;
}


PipelineStateManager::PipelineStateManager(CVkDevice* device)
	: m_device{ device }
{
	assert(g_pipelineStateManager == nullptr);

	// Populate free list and data arrays
	for (uint32_t i = 0; i < MaxItems; ++i)
	{
		m_freeList.push(i);
		m_pipelines[i].reset();
		m_descs[i] = GraphicsPipelineDesc{};
	}

	m_pipelineCache = CreatePipelineCache();

	g_pipelineStateManager = this;
}


PipelineStateManager::~PipelineStateManager()
{
	Shader::DestroyAll();

	g_pipelineStateManager = nullptr;
}


PipelineStateHandle PipelineStateManager::CreateGraphicsPipeline(const GraphicsPipelineDesc& pipelineDesc)
{
	std::lock_guard guard(m_allocationMutex);

	assert(!m_freeList.empty());

	uint32_t index = m_freeList.front();
	m_freeList.pop();

	m_descs[index] = pipelineDesc;

	m_pipelines[index] = FindOrCreateGraphicsPipelineState(pipelineDesc);

	return Create<PipelineStateHandleType>(index, this);
}


void PipelineStateManager::DestroyHandle(PipelineStateHandleType* handle)
{
	assert(handle != nullptr);

	std::lock_guard guard(m_allocationMutex);

	// TODO: queue this up to execute in one big deallocation batch, e.g. at the end of the frame

	uint32_t index = handle->GetIndex();
	m_descs[index] = GraphicsPipelineDesc{};
	m_pipelines[index].reset();

	m_freeList.push(index);
}


const GraphicsPipelineDesc& PipelineStateManager::GetDesc(PipelineStateHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_descs[index];
}


VkPipeline PipelineStateManager::GetPipeline(PipelineStateHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_pipelines[index]->Get();
}


wil::com_ptr<CVkPipeline> PipelineStateManager::FindOrCreateGraphicsPipelineState(const GraphicsPipelineDesc& pipelineDesc)
{
	// Shaders
	vector<VkPipelineShaderStageCreateInfo> shaderStages;

	auto AddShaderStageCreateInfo = [this, &shaderStages](ShaderType type, const ShaderNameAndEntry& shaderAndEntry)
		{
			if (shaderAndEntry)
			{
				Shader* shader = LoadShader(type, shaderAndEntry);
				assert(shader);
				auto shaderModule = CreateShaderModule(shader);

				VkPipelineShaderStageCreateInfo createInfo{};
				FillShaderStageCreateInfo(createInfo, *shaderModule, shader);
				shaderStages.push_back(createInfo);
			}
		};

	AddShaderStageCreateInfo(ShaderType::Vertex, pipelineDesc.vertexShader);
	AddShaderStageCreateInfo(ShaderType::Pixel, pipelineDesc.pixelShader);
	AddShaderStageCreateInfo(ShaderType::Geometry, pipelineDesc.geometryShader);
	AddShaderStageCreateInfo(ShaderType::Hull, pipelineDesc.hullShader);
	AddShaderStageCreateInfo(ShaderType::Domain, pipelineDesc.domainShader);

	// Vertex streams
	vector<VkVertexInputBindingDescription> vertexInputBindings;
	const auto& vertexStreams = pipelineDesc.vertexStreams;
	const uint32_t numStreams = (uint32_t)vertexStreams.size();
	if (numStreams > 0)
	{
		vertexInputBindings.resize(numStreams);
		for (uint32_t i = 0; i < numStreams; ++i)
		{
			vertexInputBindings[i].binding = vertexStreams[i].inputSlot;
			vertexInputBindings[i].inputRate = InputClassificationToVulkan(vertexStreams[i].inputClassification);
			vertexInputBindings[i].stride = vertexStreams[i].stride;
		}
	}

	// Vertex elements
	vector<VkVertexInputAttributeDescription> vertexAttributes;
	const auto& vertexElements = pipelineDesc.vertexElements;
	const uint32_t numElements = (uint32_t)vertexElements.size();
	if (numElements > 0)
	{
		vertexAttributes.resize(numElements);
		for (uint32_t i = 0; i < numElements; ++i)
		{
			vertexAttributes[i].binding = vertexElements[i].inputSlot;
			vertexAttributes[i].location = i;
			vertexAttributes[i].format = FormatToVulkan(vertexElements[i].format);
			vertexAttributes[i].offset = vertexElements[i].alignedByteOffset;
		}
	}

	// Vertex input layout
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{
		.sType								= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount		= (uint32_t)vertexInputBindings.size(),
		.pVertexBindingDescriptions			= vertexInputBindings.data(),
		.vertexAttributeDescriptionCount	= (uint32_t)vertexAttributes.size(),
		.pVertexAttributeDescriptions		= vertexAttributes.data()
	};

	// Input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{
		.sType						= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology					= PrimitiveTopologyToVulkan(pipelineDesc.topology),
		.primitiveRestartEnable		= pipelineDesc.indexBufferStripCut == IndexBufferStripCutValue::Disabled ? VK_FALSE : VK_TRUE
	};

	// Tessellation state
	VkPipelineTessellationStateCreateInfo tessellationInfo{
		.sType					= VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
		.patchControlPoints		= GetControlPointCount(pipelineDesc.topology)
	};

	// Viewport state
	VkPipelineViewportStateCreateInfo viewportStateInfo{
		.sType			= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount	= 1,
		.pViewports		= nullptr, // dynamic state
		.scissorCount	= 1,
		.pScissors		= nullptr  // dynamic state
	};

	// Rasterizer state
	const auto rasterizerState = pipelineDesc.rasterizerState;
	VkPipelineRasterizationStateCreateInfo rasterizerInfo
	{
		.sType						= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable			= VK_FALSE,
		.rasterizerDiscardEnable	= VK_FALSE,
		.polygonMode				= FillModeToVulkan(rasterizerState.fillMode),
		.cullMode					= CullModeToVulkan(rasterizerState.cullMode),
		.frontFace					= rasterizerState.frontCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable			= (rasterizerState.depthBias != 0 || rasterizerState.slopeScaledDepthBias != 0.0f) ? VK_TRUE : VK_FALSE,
		.depthBiasConstantFactor	= *reinterpret_cast<const float*>(&rasterizerState.depthBias),
		.depthBiasClamp				= rasterizerState.depthBiasClamp,
		.depthBiasSlopeFactor		= rasterizerState.slopeScaledDepthBias,
		.lineWidth					= 1.0f
	};

	// Multisample state
	VkPipelineMultisampleStateCreateInfo multisampleInfo{
		.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples	= GetSampleCountFlags(pipelineDesc.msaaCount),
		.sampleShadingEnable	= VK_FALSE,
		.minSampleShading		= 0.0f,
		.pSampleMask			= nullptr,
		.alphaToCoverageEnable	= pipelineDesc.blendState.alphaToCoverageEnable ? VK_TRUE : VK_FALSE,
		.alphaToOneEnable		= VK_FALSE
	};

	// Depth stencil state
	const auto& depthStencilState = pipelineDesc.depthStencilState;
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo{
		.sType					= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable		= depthStencilState.depthEnable ? VK_TRUE : VK_FALSE,
		.depthWriteEnable		= depthStencilState.depthWriteMask == DepthWrite::All ? VK_TRUE : VK_FALSE,
		.depthCompareOp			= ComparisonFuncToVulkan(depthStencilState.depthFunc),
		.depthBoundsTestEnable	= VK_FALSE,
		.stencilTestEnable		= depthStencilState.stencilEnable ? VK_TRUE : VK_FALSE,
		.front = {
			.failOp			= StencilOpToVulkan(depthStencilState.frontFace.stencilFailOp),
			.passOp			= StencilOpToVulkan(depthStencilState.frontFace.stencilPassOp),
			.depthFailOp	= StencilOpToVulkan(depthStencilState.frontFace.stencilDepthFailOp),
			.compareOp		= ComparisonFuncToVulkan(depthStencilState.frontFace.stencilFunc),
			.compareMask	= depthStencilState.stencilReadMask,
			.writeMask		= depthStencilState.stencilWriteMask,
			.reference		= 0
		},
		.back = {
			.failOp			= StencilOpToVulkan(depthStencilState.backFace.stencilFailOp),
			.passOp			= StencilOpToVulkan(depthStencilState.backFace.stencilPassOp),
			.depthFailOp	= StencilOpToVulkan(depthStencilState.backFace.stencilDepthFailOp),
			.compareOp		= ComparisonFuncToVulkan(depthStencilState.backFace.stencilFunc),
			.compareMask	= depthStencilState.stencilReadMask,
			.writeMask		= depthStencilState.stencilWriteMask,
			.reference		= 0
		},
		.minDepthBounds			= 0.0f,
		.maxDepthBounds			= 1.0f
	};

	// Blend state
	const auto& blendState = pipelineDesc.blendState;
	VkPipelineColorBlendStateCreateInfo blendStateInfo{ .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };

	array<VkPipelineColorBlendAttachmentState, 8> blendAttachments;

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
		if (rt.logicOpEnable && (VK_FALSE == blendStateInfo.logicOpEnable))
		{
			blendStateInfo.logicOpEnable = VK_TRUE;
			blendStateInfo.logicOp = LogicOpToVulkan(rt.logicOp);
		}
	}
	const uint32_t numRtvs = (uint32_t)pipelineDesc.rtvFormats.size();
	blendStateInfo.attachmentCount = numRtvs;
	blendStateInfo.pAttachments = blendAttachments.data();

	// Dynamic states
	vector<VkDynamicState> dynamicStates;
	dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
	dynamicStates.push_back(VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY);
	VkPipelineDynamicStateCreateInfo dynamicStateInfo{
		.sType				= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount	= (uint32_t)dynamicStates.size(),
		.pDynamicStates		= dynamicStates.data()
	};

	// TODO: Check for dynamic rendering support here.  Will need proper extension/feature system.
	vector<VkFormat> rtvFormats(numRtvs);
	if (numRtvs > 0)
	{
		for (uint32_t i = 0; i < numRtvs; ++i)
		{
			rtvFormats[i] = FormatToVulkan(pipelineDesc.rtvFormats[i]);
		}
	}

	const Format dsvFormat = pipelineDesc.dsvFormat;
	VkPipelineRenderingCreateInfo dynamicRenderingInfo{
		.sType						= VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
		.viewMask					= 0,
		.colorAttachmentCount		= (uint32_t)rtvFormats.size(),
		.pColorAttachmentFormats	= rtvFormats.data(),
		.depthAttachmentFormat		= FormatToVulkan(dsvFormat),
		.stencilAttachmentFormat	= IsStencilFormat(dsvFormat) ? FormatToVulkan(dsvFormat) : VK_FORMAT_UNDEFINED
	};

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{
		.sType					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext					= &dynamicRenderingInfo,
		.stageCount				= (uint32_t)shaderStages.size(),
		.pStages				= shaderStages.data(),
		.pVertexInputState		= &vertexInputInfo,
		.pInputAssemblyState	= &inputAssemblyInfo,
		.pTessellationState		= &tessellationInfo,
		.pViewportState			= &viewportStateInfo,
		.pRasterizationState	= &rasterizerInfo,
		.pMultisampleState		= &multisampleInfo,
		.pDepthStencilState		= &depthStencilInfo,
		.pColorBlendState		= &blendStateInfo,
		.pDynamicState			= &dynamicStateInfo,
		.layout					= GetVulkanRootSignaturePool()->GetPipelineLayout(pipelineDesc.rootSignature.get()),
		.renderPass				= VK_NULL_HANDLE,
		.subpass				= 0,
		.basePipelineHandle		= VK_NULL_HANDLE,
		.basePipelineIndex		= 0
	};

	VkPipeline vkPipeline{ VK_NULL_HANDLE };
	if (VK_SUCCEEDED(vkCreateGraphicsPipelines(*m_device, *m_pipelineCache, 1, &pipelineCreateInfo, nullptr, &vkPipeline)))
	{
		auto pipeline = Create<CVkPipeline>(m_device.get(), vkPipeline);
		return pipeline;
	}
	else
	{
		LogError(LogVulkan) << "Failed to create VkPipeline (graphics).  Error code: " << res << endl;
	}

	return nullptr;
}


wil::com_ptr<CVkShaderModule> PipelineStateManager::CreateShaderModule(Shader* shader)
{
	CVkShaderModule** ppShaderModule = nullptr;
	bool firstCompile = false;
	{
		lock_guard<mutex> CS(m_shaderModuleMutex);

		size_t hashCode = shader->GetHash();
		auto iter = m_shaderModuleHashMap.find(hashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == m_shaderModuleHashMap.end())
		{
			ppShaderModule = m_shaderModuleHashMap[hashCode].addressof();
			firstCompile = true;
		}
		else
		{
			ppShaderModule = &iter->second;
		}
	}

	if (firstCompile)
	{
		VkShaderModuleCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = shader->GetByteCodeSize(),
			.pCode = reinterpret_cast<const uint32_t*>(shader->GetByteCode())
		};

		VkShaderModule vkShaderModule{ VK_NULL_HANDLE };
		vkCreateShaderModule(*m_device, &createInfo, nullptr, &vkShaderModule);

		wil::com_ptr<CVkShaderModule> shaderModule = Create<CVkShaderModule>(m_device.get(), vkShaderModule);

		*ppShaderModule = shaderModule.get();

		(*ppShaderModule)->AddRef();
	}

	return *ppShaderModule;
}


wil::com_ptr<CVkPipelineCache> PipelineStateManager::CreatePipelineCache() const
{
	VkPipelineCacheCreateInfo createInfo{
		.sType				= VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
		.initialDataSize	= 0,
		.pInitialData		= nullptr
	};

	VkPipelineCache vkPipelineCache{ VK_NULL_HANDLE };
	if (VK_SUCCEEDED(vkCreatePipelineCache(*m_device, &createInfo, nullptr, &vkPipelineCache)))
	{
		return Create<CVkPipelineCache>(m_device.get(), vkPipelineCache);
	}
	else
	{
		LogError(LogVulkan) << "Failed to create VkPipelineCache.  Error code: " << res << endl;
	}

	return nullptr;
}


PipelineStateManager* const GetVulkanPipelineStateManager()
{
	return g_pipelineStateManager;
}

} // namespace Luna::VK