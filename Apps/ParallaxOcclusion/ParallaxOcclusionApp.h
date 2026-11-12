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


class ParallaxOcclusionApp : public Luna::Application
{
public:
	ParallaxOcclusionApp(uint32_t width, uint32_t height);

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
	void InitDescriptorSet();

	void LoadAssets();

	void UpdateConstantBuffer();

protected:
	struct Vertex
	{
		float pos[3];
		float normal[3];
		float tangent[3];
		float uv[2];
	};

	struct VSConstants
	{
		Math::Matrix4 projection{};
		Math::Matrix4 view{};
		Math::Matrix4 model{};
		Math::Vector4 lightPos{ 0.0f, 2.0f, 0.0f, 1.0f };
		Math::Vector4 cameraPos{};
	};

	struct PSConstants
	{
		float heightScale = 0.1f;
		float parallaxBias = -0.02f;
		float numLayers = 48.0f;
		int32_t mappingMode = 4;
	};

	VSConstants m_vsConstants{};
	PSConstants m_psConstants{};

	Luna::GpuBufferPtr m_vsConstantBuffer;
	Luna::GpuBufferPtr m_psConstantBuffer;

	Luna::RootSignaturePtr m_rootSignature;
	Luna::GraphicsPipelinePtr m_graphicsPipeline;
	bool m_pipelineCreated{ false };

	Luna::TexturePtr m_colorTexture;
	Luna::TexturePtr m_normalHeightTexture;

	Luna::DescriptorSetPtr m_descriptorSet;

	Luna::ModelPtr m_planeModel;

	Luna::CameraController m_controller{ m_camera, Math::Vector3(Math::kYUnitVector) };
};