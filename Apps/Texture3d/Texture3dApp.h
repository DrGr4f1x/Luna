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

#define APP_DYNAMIC_DESCRIPTORS 1

#include "Application.h"
#include "CameraController.h"


class Texture3dApp : public Luna::Application
{
public:
	Texture3dApp(uint32_t width, uint32_t height);

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
	void InitTexture();
#if !APP_DYNAMIC_DESCRIPTORS
	void InitDescriptorSets();
#endif // !APP_DYNAMIC_DESCRIPTORS

	void UpdateConstantBuffer();

protected:
	struct Vertex
	{
		float position[3];
		float normal[3];
		float uv[2];
	};

	struct Constants
	{
		Math::Matrix4 viewProjectionMatrix{ Math::kIdentity };
		Math::Matrix4 modelMatrix{ Math::kIdentity };
		Math::Vector4 viewPos{ Math::kZero };
		float depth{ 0.0f };
	};

	Luna::RootSignaturePtr m_rootSignature;
	Luna::GraphicsPipelinePtr m_graphicsPipeline;
	bool m_pipelineCreated{ false };

	Constants m_constants{};
	Luna::GpuBufferPtr m_constantBuffer;

#if !APP_DYNAMIC_DESCRIPTORS
	Luna::DescriptorSetPtr m_cbvDescriptorSet;
	Luna::DescriptorSetPtr m_srvDescriptorSet;
#endif // !APP_DYNAMIC_DESCRIPTORS

	Luna::GpuBufferPtr m_vertexBuffer;
	Luna::GpuBufferPtr m_indexBuffer;

	Luna::TexturePtr m_texture;

	float m_zoom{ -2.5f };
	Luna::CameraController m_controller{ m_camera, Math::Vector3(Math::kYUnitVector) };
};