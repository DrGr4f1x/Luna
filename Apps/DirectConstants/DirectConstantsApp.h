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

class DirectConstantsApp : public Luna::Application
{
public:
	DirectConstantsApp(uint32_t width, uint32_t height);

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
	void InitConstantBuffer();
	void InitResourceSet();

	void UpdateConstantBuffer();

	void LoadAssets();

protected:
	struct Constants
	{
		Math::Matrix4 projectionMatrix{ Math::kIdentity };
		Math::Matrix4 modelMatrix{ Math::kIdentity };
	};

	struct LightConstants
	{
		Math::Vector4 lightPositions[6];
	};

	Luna::RootSignaturePtr m_rootSignature;
	Luna::GraphicsPipelinePtr m_pipeline;
	bool m_pipelineCreated{ false };

	Constants m_constants{};
	Luna::GpuBufferPtr m_constantBuffer;

	LightConstants m_lightConstants{};

	Luna::ResourceSet m_resources;

	Luna::ModelPtr m_model;

	Luna::CameraController m_controller;
};