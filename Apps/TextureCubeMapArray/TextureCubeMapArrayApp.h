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


class TextureCubeMapArrayApp : public Luna::Application
{
public:
	TextureCubeMapArrayApp(uint32_t width, uint32_t height);

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
	void InitConstantBuffers();
	void InitResourceSets();

	void UpdateConstantBuffers();

	void LoadAssets();

protected:
	struct Vertex
	{
		float position[3];
		float normal[3];
		float uv[2];
	};

	struct VSConstants
	{
		Math::Matrix4 viewProjectionMatrix{ Math::kIdentity };
		Math::Matrix4 modelMatrix{ Math::kIdentity };
		Math::Vector3 eyePos{ Math::kZero };
	};

	struct PSConstants
	{
		float lodBias{ 0.0f };
		int arraySlice{ 1 };
	};

	Luna::RootSignaturePtr m_rootSignature;

	Luna::GraphicsPipelinePtr m_modelPipeline;
	Luna::GraphicsPipelinePtr m_skyboxPipeline;
	bool m_pipelinesCreated{ false };

	VSConstants m_vsSkyboxConstants{};
	VSConstants	m_vsModelConstants{};
	PSConstants	m_psConstants{};

	Luna::GpuBufferPtr m_vsSkyboxConstantBuffer;
	Luna::GpuBufferPtr m_vsModelConstantBuffer;
	Luna::GpuBufferPtr m_psConstantBuffer;

	Luna::ResourceSet m_modelResources;
	Luna::ResourceSet m_skyboxResources;

	Luna::TexturePtr m_skyboxTex;
	Luna::ModelPtr m_skyboxModel;
	Luna::SamplerPtr m_sampler;
	std::vector<Luna::ModelPtr> m_models;
	std::vector<std::string> m_modelNames;
	int32_t	m_curModel{ 0 };
	bool m_displaySkybox{ true };

	Luna::CameraController m_controller;
};