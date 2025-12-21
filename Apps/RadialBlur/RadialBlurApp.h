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

	void InitRootSignatures();
	void InitPipelines();
	void InitRenderTargets();
	void InitDescriptorSets();

	void LoadAssets();

	void UpdateSceneConstantBuffer();
	void UpdateRadialBlurConstantBuffer();

protected:
	// Render pipeline resources
	struct SceneConstants
	{
		Math::Matrix4 projectionMat{ Math::kIdentity };
		Math::Matrix4 modelMat{ Math::kIdentity };
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

	Luna::RootSignaturePtr m_radialBlurRootSignature;
	Luna::RootSignaturePtr m_sceneRootSignature;

	Luna::GraphicsPipelinePtr m_radialBlurPipeline;
	Luna::GraphicsPipelinePtr m_colorPassPipeline;
	Luna::GraphicsPipelinePtr m_phongPassPipeline;
	Luna::GraphicsPipelinePtr m_displayTexturePipeline;
	bool m_pipelinesCreated{ false };

	// Constant buffers
	SceneConstants m_sceneConstants{};
	Luna::GpuBufferPtr m_sceneConstantBuffer;

	RadialBlurConstants m_radialBlurConstants{};
	Luna::GpuBufferPtr m_radialBlurConstantBuffer;

	// Descriptor sets
	Luna::DescriptorSetPtr m_sceneSrvDescriptorSet;
	Luna::DescriptorSetPtr m_blurCbvSrvDescriptorSet;

	// Assets
	Luna::ModelPtr m_model;
	Luna::TexturePtr m_gradientTex;

	// Camera controls
	float m_zoom{ -10.0f };
	Math::Vector3 m_cameraPos{ Math::kZero };

	// Features
	bool m_blur{ true };
	bool m_displayTexture{ false };
	Math::Vector3 m_rotation{ -16.25f, -28.75f, 0.0f };
};