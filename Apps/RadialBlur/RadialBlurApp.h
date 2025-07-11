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

class RadialBlurApp : public Luna::Application
{
public:
	RadialBlurApp(uint32_t width, uint32_t height);

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
	void InitRenderTargets();
	void InitConstantBuffers();
	void InitResourceSets();

	void LoadAssets();

	void UpdateSceneConstantBuffer();
	void UpdateRadialBlurConstantBuffer();

protected:
	// Render pipeline resources
	struct Vertex
	{
		float position[3];
		float normal[3];
		float color[4];
		float uv[2];
	};
	static_assert(sizeof(Vertex) == 48);

	struct SceneConstants
	{
		Math::Matrix4 projectionMat;
		Math::Matrix4 modelMat;
		float gradientPos{ 0.0f };
	};

	struct RadialBlurConstants
	{
		float radialBlurScale = { 0.35f };
		float radialBlurStrength = { 0.75f };
		float radialOrigin[2] = { 0.5f, 0.5f };
	};

	static const uint32_t s_offscreenSize{ 512 };

	Luna::ColorBufferPtr m_offscreenColorBuffer;
	Luna::DepthBufferPtr m_offscreenDepthBuffer;
	Luna::DepthBufferPtr m_depthBuffer;

	Luna::RootSignaturePtr m_radialBlurRootSignature;
	Luna::RootSignaturePtr m_sceneRootSignature;

	Luna::GraphicsPipelineStatePtr m_radialBlurPipeline;
	Luna::GraphicsPipelineStatePtr m_colorPassPipeline;
	Luna::GraphicsPipelineStatePtr m_phongPassPipeline;
	Luna::GraphicsPipelineStatePtr m_displayTexturePipeline;
	bool m_pipelinesCreated{ false };

	// Constant buffers
	SceneConstants m_sceneConstants;
	Luna::GpuBufferPtr m_sceneConstantBuffer;

	RadialBlurConstants m_radialBlurConstants;
	Luna::GpuBufferPtr m_radialBlurConstantBuffer;

	// Resource sets
	Luna::ResourceSet m_sceneResources;
	Luna::ResourceSet m_blurResources;

	// Assets
	Luna::ModelPtr m_model;
	Luna::TexturePtr m_gradientTex;
	Luna::SamplerPtr m_samplerLinearWrap;
	Luna::SamplerPtr m_samplerLinearClamp;

	// Camera controls
	float m_zoom{ -10.0f };
	Math::Vector3 m_cameraPos{ Math::kZero };

	// Features
	bool m_blur{ true };
	bool m_displayTexture{ false };
	Math::Vector3 m_rotation{ -16.25f, -28.75f, 0.0f };
};