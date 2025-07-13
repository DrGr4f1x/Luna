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
#include "CameraController.h"
#include "Graphics\DepthBuffer.h"
#include "Graphics\GpuBuffer.h"
#include "Graphics\PipelineState.h"
#include "Graphics\ResourceSet.h"
#include "Graphics\RootSignature.h"
#include "Graphics\Texture.h"


class TextureApp : public Luna::Application
{
public:
	TextureApp(uint32_t width, uint32_t height);
	~TextureApp() = default;

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

	void InitDepthBuffer();
	void InitRootSignature();
	void InitPipelineState();
	void InitResources();
	void LoadAssets();

	void UpdateConstantBuffer();

protected:
	// Vertex layout for this example
	struct Vertex
	{
		float position[3];
		float uv[2];
		float normal[3];
	};

	struct Constants
	{
		Math::Matrix4 viewProjectionMatrix;
		Math::Matrix4 modelMatrix;
		Math::Vector3 viewPos;
		float lodBias{ 0.0f };
		bool flipUVs{ false };
	};

	Luna::DepthBufferPtr m_depthBuffer;

	Luna::GpuBufferPtr m_vertexBuffer;
	Luna::GpuBufferPtr m_indexBuffer;

	Constants m_constants;
	Luna::GpuBufferPtr m_constantBuffer;

	Luna::ResourceSet m_resources;

	Luna::RootSignaturePtr m_rootSignature;
	Luna::GraphicsPipelinePtr m_graphicsPipeline;
	bool m_pipelineCreated{ false };

	// Assets
	Luna::TexturePtr m_texture;
	Luna::SamplerPtr m_sampler;
	bool m_flipUVs{ false };

	float m_zoom{ -2.5f };
	Luna::CameraController m_controller;
};