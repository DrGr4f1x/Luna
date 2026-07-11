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


class VariableRateShadingApp : public Luna::Application
{
public:
	VariableRateShadingApp(uint32_t width, uint32_t height);

	int ProcessCommandLine(int argc, char* argv[]) final;

	void Configure() final;
	void Startup() final;
	void Shutdown() final;

	void Update() final;
	void Render() final;

protected:
	void CreateDeviceDependentResources() final;
	void CreateWindowSizeDependentResources() final;

	void InitRootSignature();
	void InitPipeline();

	void LoadAssets();

protected:
	struct Vertex
	{
		float pos[3];
		float normal[3];
		float uv[2];
		float tangent[3];
	};

	Luna::GpuBufferPtr m_vertexBuffer;
	Luna::GpuBufferPtr m_indexBuffer;

	Luna::RootSignaturePtr m_sceneRootSignature;

	Luna::GraphicsPipelinePtr m_sceneGraphicsPipeline;
	bool m_pipelineCreated{ false };

	Math::BoundingBox m_sceneBoundingBox{};

	std::vector<Luna::TexturePtr> m_textures;
};