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

#include "Application.h"

class ComputeShaderApp : public Luna::Application
{
public:
	ComputeShaderApp(uint32_t width, uint32_t height);

	int ProcessCommandLine(int argc, char* argv[]) final;

	void Configure() final;
	void Startup() final;
	void Shutdown() final;

	void Update() final;
	void UpdateUI() final;
	void Render() final;

protected:
	void CreateDeviceDependentResources() final;
	void CreateWindowSizeDependentResources() final;

	void InitRootSignatures();
	void InitPipelines();
	void InitConstantBuffer();
	void InitResourceSets();

	void LoadAssets();

protected:
	// Vertex layout for this example
	struct Vertex
	{
		float position[3];
		float uv[2];
	};

	struct Constants
	{
		Math::Matrix4 viewProjectionMatrix{ Math::kIdentity };
		Math::Matrix4 modelMatrix{ Math::kIdentity };
	};

	Luna::GpuBufferPtr m_vertexBuffer;
	Luna::GpuBufferPtr m_indexBuffer;

	Constants m_constants{};
	Luna::GpuBufferPtr m_constantBuffer;

	Luna::RootSignaturePtr m_graphicsrootSignature;
	Luna::RootSignaturePtr m_computeRootSignature;

	Luna::GraphicsPipelinePtr m_graphicsPipeline;
	Luna::ComputePipelinePtr m_edgeDetectPipeline;
	Luna::ComputePipelinePtr m_embossPipeline;
	Luna::ComputePipelinePtr m_sharpenPipeline;
	bool m_pipelinesCreated{ false };

	Luna::ResourceSet m_computeResources;
	Luna::ResourceSet m_gfxLeftResources;
	Luna::ResourceSet m_gfxRightResources;

	Luna::ColorBufferPtr m_computeScratchBuffer;

	Luna::TexturePtr m_texture;
	Luna::SamplerPtr m_sampler;

	std::vector<std::string> m_shaderNames;
	int32_t	m_curComputeTechnique{ 0 };
};