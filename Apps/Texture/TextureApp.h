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


class TextureApp : public Luna::Application
{
public:
	TextureApp(uint32_t width, uint32_t height);
	~TextureApp() = default;

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
	void InitPipelineState();
	void InitDescriptorSets();
	void LoadAssets();

	void UpdateConstantBuffer();

protected:
	// Vertex layout for this example
	struct Vertex
	{
		float position[3];
		float uv[2];
		float normal[3];
	};

	struct Constants
	{
		Math::Matrix4 viewProjectionMatrix{ Math::kIdentity };
		Math::Matrix4 modelMatrix{ Math::kIdentity };
		Math::Vector3 viewPos{ Math::kZero };
		float lodBias{ 0.0f };
		bool flipUVs{ false };
	};

	Luna::GpuBufferPtr m_vertexBuffer;
	Luna::GpuBufferPtr m_indexBuffer;

	Constants m_constants{};
	Luna::GpuBufferPtr m_constantBuffer;

	Luna::DescriptorSetPtr m_cbvDescriptorSet;
	Luna::DescriptorSetPtr m_srvDescriptorSet;

	Luna::RootSignaturePtr m_rootSignature;
	Luna::GraphicsPipelinePtr m_graphicsPipeline;
	bool m_pipelineCreated{ false };

	// Assets
	Luna::TexturePtr m_texture;
	bool m_flipUVs{ false };

	float m_zoom{ -2.5f };
	Luna::CameraController m_controller{ m_camera, Math::Vector3(Math::kYUnitVector) };
};