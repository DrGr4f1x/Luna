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


class MultisamplingApp : public Luna::Application
{
public:
	MultisamplingApp(uint32_t width, uint32_t height);

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

	void InitRenderTargets();
	void InitRootSignature();
	void InitPipelines();
	void InitConstantBuffer();
	void InitResources();

	void LoadAssets();

	void UpdateConstantBuffer();

protected:
	struct Vertex
	{
		float position[3];
		float normal[3];
		float color[4];
		float uv[2];
	};

	struct Constants
	{
		Math::Matrix4 viewProjectionMatrix;
		Math::Matrix4 modelMatrix;
		Math::Vector4 lightPos;
	};

	Luna::ColorBufferPtr m_msaaColorBuffer;
	Luna::DepthBufferPtr m_msaaDepthBuffer;
	const uint32_t m_numSamples{ 8 };

	Luna::DepthBufferPtr m_depthBuffer;

	Constants m_constants;
	Luna::GpuBufferPtr m_constantBuffer;

	std::vector<Luna::ResourceSet> m_resources;

	Luna::RootSignaturePtr m_rootSignature;
	Luna::GraphicsPipelinePtr m_msaaPipeline;
	Luna::GraphicsPipelinePtr m_msaaSampleRatePipeline;
	bool m_pipelinesCreated{ false };

	Luna::ModelPtr m_model;
	Luna::SamplerPtr m_sampler;

	Luna::CameraController	m_controller;

	bool m_sampleRateShading{ false };
};