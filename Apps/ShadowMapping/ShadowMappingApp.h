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


class ShadowMappingApp : public Luna::Application
{
public:
	ShadowMappingApp(uint32_t width, uint32_t height);

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
	void InitShadowMap();
	void InitConstantBuffers();
	void InitDescriptorSets();

	void LoadAssets();

	void UpdateConstantBuffers();

protected:
	struct SceneConstants
	{
		Math::Matrix4 projectionMatrix{ Math::kIdentity };
		Math::Matrix4 viewMatrix{ Math::kIdentity };
		Math::Matrix4 modelMatrix{ Math::kIdentity };
		Math::Matrix4 depthBiasModelViewProjectionMatrix{ Math::kIdentity };
		Math::Vector4 lightPosition{ Math::kZero };
		float zNear{ 0.0f };
		float zFar{ 1.0f };
	};

	struct ShadowConstants
	{
		Math::Matrix4 modelViewProjectionMatrix;
	};

	SceneConstants m_sceneConstants{};
	ShadowConstants m_shadowConstants{};

	Luna::GpuBufferPtr m_sceneConstantBuffer;
	Luna::GpuBufferPtr m_shadowConstantBuffer;

	Luna::DepthBufferPtr m_shadowMap;

	Luna::RootSignaturePtr m_shadowDepthRootSignature;
	Luna::RootSignaturePtr m_sceneRootSignature;
	Luna::RootSignaturePtr m_shadowVisualizationRootSignature;

	Luna::GraphicsPipelinePtr m_shadowDepthPipeline;
	Luna::GraphicsPipelinePtr m_scenePipeline;
	Luna::GraphicsPipelinePtr m_shadowVisualizationPipeline;
	bool m_pipelinesCreated{ false };

	Luna::DescriptorSetPtr m_scenePsDescriptorSet;
	Luna::DescriptorSetPtr m_shadowVisualizationDescriptorSet;

	std::vector<Luna::ModelPtr> m_scenes;
	std::vector<std::string> m_sceneNames;

	const uint32_t m_shadowMapSize{ 2048 };
	float m_zNear{ 1.0f };
	float m_zFar{ 96.0f };
	float m_lightFOV{ 45.0f };
	Math::Vector3 m_lightPos{ Math::kZero };

	int32_t m_sceneIndex{ 0 };
	bool m_visualizeShadowMap{ false };
	bool m_usePCF{ true };

	Luna::CameraController m_controller{ m_camera, Math::Vector3(Math::kYUnitVector) };
};