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


class StencilBufferApp : public Luna::Application
{
public:
	StencilBufferApp(uint32_t width, uint32_t height);

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

	void InitRootSignature();
	void InitPipelines();
	void InitResourceSet();

	void UpdateConstantBuffer();

	void LoadAssets();

protected:
	struct Vertex
	{
		float position[3];
		float color[4];
		float normal[3];
	};

	struct Constants
	{
		Math::Matrix4 viewProjectionMatrix{ Math::kIdentity };
		Math::Matrix4 modelMatrix{ Math::kIdentity };
		Math::Vector4 lightPos{ 0.0f, -2.0f, 1.0f, 0.0f };
		float outlineWidth = 0.05f;
	};

	Luna::RootSignaturePtr m_rootSignature;
	Luna::GraphicsPipelinePtr m_toonPipeline;
	Luna::GraphicsPipelinePtr m_outlinePipeline;
	bool m_pipelinesCreated{ false };

	Luna::GpuBufferPtr m_constantBuffer;
	Constants m_constants;

	Luna::ResourceSet m_resources;

	Luna::ModelPtr m_model;

	Luna::CameraController m_controller;
};