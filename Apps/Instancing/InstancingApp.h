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


class InstancingApp : public Luna::Application
{
public:
	InstancingApp(uint32_t width, uint32_t height);

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
	void InitInstanceBuffer();
	void InitDescriptorSets();

	void UpdateConstantBuffer();

	void LoadAssets();

protected:
	struct VSConstants
	{
		Math::Matrix4 projectionMatrix{ Math::kIdentity };
		Math::Matrix4 modelViewMatrix{ Math::kIdentity };
		Math::Vector4 lightPos{ 0.0f, -5.0f, 0.0f, 1.0f };
		float localSpeed{ 0.0f };
		float globalSpeed{ 0.0f };
	};

	VSConstants m_vsConstants{};

	Luna::GpuBufferPtr m_vsConstantBuffer;

	Luna::GpuBufferPtr m_instanceBuffer;

	Luna::RootSignaturePtr m_starfieldRootSignature;
	Luna::RootSignaturePtr m_modelRootSignature;

	Luna::GraphicsPipelinePtr m_starfieldPipeline;
	Luna::GraphicsPipelinePtr m_rockPipeline;
	Luna::GraphicsPipelinePtr m_planetPipeline;
	bool m_pipelinesCreated{ false };

	Luna::TexturePtr m_rockTexture;
	Luna::TexturePtr m_planetTexture;

	Luna::ModelPtr m_rockModel;
	Luna::ModelPtr m_planetModel;

	Luna::DescriptorSetPtr m_rockSrvDescriptorSet;
	Luna::DescriptorSetPtr m_planetSrvDescriptorSet;

	Luna::CameraController m_controller{ m_camera, Math::Vector3(Math::kYUnitVector) };
	float m_zoom{ -18.5 };
	float m_rotationSpeed{ 0.25f };

	const uint32_t m_numInstances{ 8192 };
};