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


class DeferredApp : public Luna::Application
{
public:
	DeferredApp(uint32_t width, uint32_t height);

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

	void InitGBuffer();
	void InitRootSignatures();
	void InitPipelines();
	void InitConstantBuffers();
	void InitResourceSets();

	void LoadAssets();

	void UpdateConstantBuffers();

protected:
	// G-Buffer
	Luna::ColorBufferPtr m_positionBuffer;
	Luna::ColorBufferPtr m_normalBuffer;
	Luna::ColorBufferPtr m_albedoBuffer;
	Luna::DepthBufferPtr m_depthBuffer;

	// Root signatures and pipelines
	Luna::RootSignaturePtr m_gbufferRootSignature;
	Luna::RootSignaturePtr m_lightingRootSignature;
	Luna::GraphicsPipelinePtr m_gbufferPipeline;
	Luna::GraphicsPipelinePtr m_lightingPipeline;
	bool m_pipelinesCreated{ false };

	// G-Buffer vertex
	struct Vertex
	{
		float position[3];
		float normal[3];
		float tangent[3];
		float color[4];
		float uv[2];
	};

	// G-Buffer pass constants
	struct GBufferConstants
	{
		Math::Matrix4 projectionMatrix{ Math::kIdentity };
		Math::Matrix4 modelMatrix{ Math::kIdentity };
		Math::Matrix4 viewMatrix{ Math::kIdentity };
		Math::Vector3 instancePositions[3];
	};
	GBufferConstants m_gbufferConstants{};

	// Light struct
	struct Light
	{
		Math::Vector4 position{ Math::kIdentity };
		Math::Vector4 colorAndRadius{ Math::kZero };
	};

	// Lighting pass constants
	struct LightingConstants
	{
		Light lights[6];
		Math::Vector4 viewPosition{ Math::kZero };
		int displayBuffer{ 0 };
	};
	LightingConstants m_lightingConstants{};

	// Constant buffers
	Luna::GpuBufferPtr m_gbufferConstantBuffer;
	Luna::GpuBufferPtr m_lightingConstantBuffer;

	// Resource sets
	Luna::ResourceSet m_armorResources;
	Luna::ResourceSet m_floorResources;
	Luna::ResourceSet m_lightingResources;

	// Assets
	Luna::ModelPtr m_armorModel;
	Luna::TexturePtr m_armorColorTexture;
	Luna::TexturePtr m_armorNormalTexture;

	Luna::ModelPtr m_floorModel;
	Luna::TexturePtr m_floorColorTexture;
	Luna::TexturePtr m_floorNormalTexture;
	
	Luna::CameraController m_controller{ m_camera, Math::Vector3(Math::kYUnitVector) };
	int32_t m_displayBuffer{ 0 };
};