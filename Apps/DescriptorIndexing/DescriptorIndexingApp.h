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


class DescriptorIndexingApp : public Luna::Application
{
public:
	DescriptorIndexingApp(uint32_t width, uint32_t height);

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
	void InitDescriptorSets();

	void GenerateTextures();
	void GenerateCubes();

	void UpdateConstantBuffer();

protected:
	struct Constants
	{
		Math::Matrix4 projectionMatrix{ Math::kIdentity };
		Math::Matrix4 viewMatrix{ Math::kIdentity };
		Math::Matrix4 modelMatrix{ Math::kIdentity };
	};

	struct Vertex 
	{
		float pos[3];
		float uv[2];
		int32_t textureIndex{ 0 };
	};

	Constants m_constants;
	Luna::GpuBufferPtr m_constantBuffer;

	Luna::RootSignaturePtr m_rootSignature;
	Luna::GraphicsPipelinePtr m_graphicsPipeline;
	bool m_pipelineCreated{ false };

	Luna::GpuBufferPtr m_vertexBuffer;
	Luna::GpuBufferPtr m_indexBuffer;

	Luna::DescriptorSetPtr m_vsDescriptorSet;
	Luna::DescriptorSetPtr m_psDescriptorSet;

	std::vector<Luna::TexturePtr> m_textures;
	const uint32_t m_numTextures{ 32 };
	const uint32_t m_numCubes{ 5 };

	Luna::CameraController m_controller{ m_camera, Math::Vector3(Math::kYUnitVector) };
};