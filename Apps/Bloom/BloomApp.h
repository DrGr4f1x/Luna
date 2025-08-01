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


class BloomApp : public Luna::Application
{
public:
	BloomApp(uint32_t width, uint32_t height);

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

	void InitOffscreenBuffers();
	void InitRootSignatures();
	void InitPipelines();
	void InitConstantBuffers();
	void InitDescriptorSets();

	void LoadAssets();

	void UpdateConstantBuffers();
	void UpdateBlurConstants();

protected:
	struct Vertex
	{
		float position[3];
		float normal[3];
		float color[4];
		float uv[2];
	};

	struct SceneConstants
	{
		Math::Matrix4 projectionMat{ Math::kIdentity };
		Math::Matrix4 viewMat{ Math::kIdentity };
		Math::Matrix4 modelMat{ Math::kIdentity };
	};

	struct BlurConstants
	{
		float blurScale{ 0.0f };
		float blurStrength{ 0.0f };
		int blurDirection{ 0 };
	};

	Luna::ColorBufferPtr m_offscreenColorBuffer[2];
	Luna::DepthBufferPtr m_offscreenDepthBuffer;
	uint32_t m_offscreenBufferSize{ 256 };

	Luna::RootSignaturePtr m_sceneRootSignature;
	Luna::RootSignaturePtr m_blurRootSignature;
	Luna::RootSignaturePtr m_skyboxRootSignature;

	Luna::GraphicsPipelinePtr m_colorPassPipeline;
	Luna::GraphicsPipelinePtr m_phongPassPipeline;
	Luna::GraphicsPipelinePtr m_blurVertPipeline;
	Luna::GraphicsPipelinePtr m_blurHorizPipeline;
	Luna::GraphicsPipelinePtr m_skyboxPipeline;
	bool m_pipelinesCreated{ false };

	// Constant buffers
	SceneConstants m_sceneConstants{};
	Luna::GpuBufferPtr m_sceneConstantBuffer;

	SceneConstants m_skyboxConstants{};
	Luna::GpuBufferPtr m_skyboxConstantBuffer;

	BlurConstants m_blurHorizConstants{};
	Luna::GpuBufferPtr m_blurHorizConstantBuffer;
	BlurConstants m_blurVertConstants{};
	Luna::GpuBufferPtr m_blurVertConstantBuffer;

	// Descriptor sets
	Luna::DescriptorSetPtr m_sceneCbvDescriptorSet;
	Luna::DescriptorSetPtr m_skyBoxCbvDescriptorSet;
	Luna::DescriptorSetPtr m_skyBoxSrvDescriptorSet;
	Luna::DescriptorSetPtr m_blurHorizDescriptorSet;
	Luna::DescriptorSetPtr m_blurVertDescriptorSet;

	// Assets
	Luna::ModelPtr m_ufoModel;
	Luna::ModelPtr m_ufoGlowModel;
	Luna::ModelPtr m_skyboxModel;
	Luna::TexturePtr m_skyboxTexture;

	Luna::CameraController m_controller{ m_camera, Math::Vector3(Math::kYUnitVector) };

	bool m_bloom{ true };
	float m_blurScale{ 1.0f };
};