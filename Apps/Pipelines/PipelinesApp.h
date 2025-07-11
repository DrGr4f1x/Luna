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

class PipelinesApp : public Luna::Application
{
public:
	PipelinesApp(uint32_t width, uint32_t height);

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
	void InitPipelines();
	void InitConstantBuffer();
	void InitResourceSet();

	void UpdateConstantBuffer();

	void LoadAssets();

protected:
	struct Vertex
	{
		float position[3];
		float normal[3];
		float color[4];
		float uv[2];
	};

	struct VSConstants
	{
		Math::Matrix4 projectionMatrix;
		Math::Matrix4 modelMatrix;
		Math::Vector4 lightPos;
	};

	Luna::DepthBufferPtr m_depthBuffer;

	Luna::RootSignaturePtr m_rootSignature;
	Luna::GraphicsPipelineStatePtr m_phongPipeline;
	Luna::GraphicsPipelineStatePtr m_toonPipeline;
	Luna::GraphicsPipelineStatePtr m_wireframePipeline;
	bool m_pipelinesCreated{ false };

	VSConstants m_vsConstants;
	Luna::GpuBufferPtr m_constantBuffer;

	Luna::ResourceSet m_resources;

	Luna::ModelPtr m_model;

	Luna::CameraController m_controller;
};