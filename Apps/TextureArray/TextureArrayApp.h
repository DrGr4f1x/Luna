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


class TextureArrayApp : public Luna::Application
{
public:
	TextureArrayApp(uint32_t width, uint32_t height);

	int ProcessCommandLine(int argc, char* argv[]) final;

	void Configure() final;
	void Startup() final;
	void Shutdown() final;

	void Update() final;
	void Render() final;

protected:
	void CreateDeviceDependentResources() final;
	void CreateWindowSizeDependentResources() final;

	void InitDepthBuffer();
	void InitRootSignature();
	void InitGraphicsPipeline();
	void InitConstantBuffer();
	void InitResourceSet();

	void UpdateConstantBuffer();

	void LoadAssets();

protected:
	// Vertex layout for this example
	struct Vertex
	{
		float position[3];
		float uv[2];
	};

	struct InstanceData
	{
		// Model matrix
		Math::Matrix4 modelMatrix;
		// Texture array index
		// Vec4 due to padding
		Math::Vector4 arrayIndex;
	};

	struct Constants
	{
		// Global matrix
		Math::Matrix4 viewProjectionMatrix;

		// Separate data for each instance
		InstanceData* instance{ nullptr };
	};
	Constants m_constants;

	Luna::DepthBufferPtr m_depthBuffer;

	Luna::GpuBufferPtr m_vertexBuffer;
	Luna::GpuBufferPtr m_indexBuffer;
	Luna::GpuBufferPtr m_constantBuffer;

	Luna::ResourceSet m_resources;

	Luna::RootSignaturePtr m_rootSignature;
	Luna::GraphicsPipelinePtr m_graphicsPipeline;
	bool m_pipelineCreated{ false };

	// Assets
	Luna::TexturePtr m_texture;
	Luna::SamplerPtr m_sampler;
	uint32_t m_layerCount{ 0 };

	// Camera controls
	float m_zoom{ -15.0f };
	Math::Vector3 m_rotation{ -15.0f, 35.0f, 0.0f };
	Luna::CameraController m_controller;
};