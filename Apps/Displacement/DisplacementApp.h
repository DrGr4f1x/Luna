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

	void InitRootSignature();
	void InitPipelines();
	void InitConstantBuffers();
	void InitDescriptorSets();

	void UpdateConstantBuffers();

	void LoadAssets();

protected:
	struct HSConstants
	{
		float tessLevel{ 0.0f };
	};

	struct DSConstants
	{
		Math::Matrix4 projectionMatrix{ Math::kIdentity };
		Math::Matrix4 modelMatrix{ Math::kIdentity };
		Math::Vector4 lightPos{ Math::kZero };
		float tessAlpha{ 0.0f };
		float tessStrength{ 0.0f };
	};

	Luna::RootSignaturePtr m_rootSignature;
	Luna::GraphicsPipelinePtr m_pipeline;
	Luna::GraphicsPipelinePtr m_wireframePipeline;
	bool m_pipelinesCreated{ false };

	HSConstants m_hsConstants{};
	Luna::GpuBufferPtr m_hsConstantBuffer;

	DSConstants m_dsConstants{};
	Luna::GpuBufferPtr m_dsConstantBuffer;

	Luna::DescriptorSetPtr m_hsCbvDescriptorSet;
	Luna::DescriptorSetPtr m_dsCbvSrvDescriptorSet;
	Luna::DescriptorSetPtr m_psSrvDescriptorSet;

	Luna::TexturePtr m_texture;
	Luna::ModelPtr m_model;

	Luna::CameraController m_controller{ m_camera, Math::Vector3(Math::kYUnitVector) };

	// App features
	bool m_split{ true };
	bool m_displacement{ true };
};