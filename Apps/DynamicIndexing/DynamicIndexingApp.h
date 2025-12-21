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


class DynamicIndexingApp : public Luna::Application
{
public:
	DynamicIndexingApp(uint32_t width, uint32_t height);

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
	void InitConstantBuffer();
	void InitDescriptorSets();

	void LoadAssets();

	void UpdateConstantBuffer();

protected:
	struct Vertex
	{
		float pos[3];
		float normal[3];
		float uv[2];
		float tangent[3];
	};

	Math::Matrix4* modelViewProjectionMatrices{ nullptr };
	size_t m_dynamicAlignment{ 0 };
	
	std::vector<Math::Matrix4> m_modelMatrices;

	Luna::RootSignaturePtr m_rootSignature;
	Luna::GraphicsPipelinePtr m_graphicsPipeline;
	bool m_pipelineCreated{ false };

	Luna::GpuBufferPtr m_vertexBuffer;
	Luna::GpuBufferPtr m_indexBuffer;

	Luna::GpuBufferPtr m_constantBuffer;

	Luna::DescriptorSetPtr m_psDescriptorSet;

	Luna::TexturePtr m_diffuseTexture;
	std::vector<Luna::TexturePtr> m_cityMaterialTextures;

	Luna::CameraController m_controller{ m_camera, Math::Vector3(Math::kYUnitVector) };

	Math::BoundingBox m_modelBoundingBox{};
	Math::BoundingBox m_sceneBoundingBox{};

	// Asset info - this is hard-coded in occcity.h in Microsoft's sample
	const uint32_t m_standardVertexStride = 44;
	const uint32_t m_vertexDataOffset = 524288;
	const uint32_t m_vertexDataSize = 820248;
	const uint32_t m_indexDataOffset = 1344536;
	const uint32_t m_indexDataSize = 74568;
	const uint32_t m_textureWidth = 1024;
	const uint32_t m_textureDataSize = 524288;
	const Luna::Format m_textureFormat = Luna::Format::BC1_UNorm;

	// Scene info
	const uint32_t m_cityRowCount = 15;
	const uint32_t m_cityColumnCount = 8;
	const uint32_t m_cityMaterialCount = m_cityRowCount * m_cityColumnCount;
	const uint32_t m_cityMaterialTextureWidth = 64;
	const uint32_t m_cityMaterialTextureHeight = 64;
	const float m_citySpacingInterval = 16.0f;
};