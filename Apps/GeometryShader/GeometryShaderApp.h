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

class GeometryShaderApp : public Luna::Application
{
public:
	GeometryShaderApp(uint32_t width, uint32_t height);

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
	void InitRootSignatures();
	void InitPipelines();
	void InitConstantBuffer();
	void InitResourceSets();

	void UpdateConstantBuffer();

	void LoadAssets();

protected:
	struct Vertex
	{
		float position[3];
		float normal[3];
		float color[4];
	};

	struct Constants
	{
		Math::Matrix4 projectionMatrix;
		Math::Matrix4 modelMatrix;
	};

	Luna::DepthBufferPtr m_depthBuffer;

	Luna::RootSignaturePtr m_meshRootSignature;
	Luna::RootSignaturePtr m_geomRootSignature;

	Luna::GraphicsPipelineStatePtr m_meshPipeline;
	Luna::GraphicsPipelineStatePtr m_geomPipeline;
	bool m_pipelinesCreated{ false };

	Luna::GpuBufferPtr m_constantBuffer;
	Constants m_constants;

	Luna::ResourceSet m_meshResources;
	Luna::ResourceSet m_geomResources;

	Luna::ModelPtr m_model;

	Luna::CameraController m_controller;

	bool m_showNormals{ true };
};