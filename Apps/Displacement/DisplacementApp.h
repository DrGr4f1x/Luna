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

class DisplacementApp : public Luna::Application
{
public:
	DisplacementApp(uint32_t width, uint32_t height);

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
	void InitConstantBuffers();
	void InitResourceSet();

	void UpdateConstantBuffers();

	void LoadAssets();

protected:
	struct HSConstants
	{
		float tessLevel;
	};

	struct DSConstants
	{
		Math::Matrix4 projectionMatrix;
		Math::Matrix4 modelMatrix;
		Math::Vector4 lightPos;
		float tessAlpha;
		float tessStrength;
	};

	Luna::DepthBufferPtr m_depthBuffer;

	Luna::RootSignaturePtr m_rootSignature;
	Luna::GraphicsPipelinePtr m_pipeline;
	Luna::GraphicsPipelinePtr m_wireframePipeline;
	bool m_pipelinesCreated{ false };

	Luna::GpuBufferPtr m_hsConstantBuffer;
	HSConstants m_hsConstants;

	Luna::GpuBufferPtr m_dsConstantBuffer;
	DSConstants m_dsConstants;

	Luna::ResourceSet m_resources;

	Luna::TexturePtr m_texture;
	Luna::ModelPtr m_model;
	Luna::SamplerPtr m_sampler;

	Luna::CameraController m_controller;

	// App features
	bool m_split{ true };
	bool m_displacement{ true };
};