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


class TerrainTessellationApp : public Luna::Application
{
public:
	TerrainTessellationApp(uint32_t width, uint32_t height);

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
	void InitConstantBuffers();
	void InitResourceSets();
	void InitTerrain();

	void LoadAssets();

	void UpdateConstantBuffers();

protected:
	struct Vertex
	{
		float position[3];
		float normal[3];
		float uv[2];
	};

	struct SkyConstants
	{
		Math::Matrix4 modelViewProjectionMatrix{ Math::kIdentity };
	};

	struct TerrainConstants
	{
		Math::Matrix4 projectionMatrix{ Math::kIdentity };
		Math::Matrix4 modelViewMatrix{ Math::kIdentity };
		Math::Vector4 lightPos{ Math::kZero };
		Math::BoundingPlane frustumPlanes[6];
		float viewportDim[2];
		float displacementFactor;
		float tessellationFactor;
		float tessellatedEdgeSize;
	};

	Luna::RootSignaturePtr m_skyRootSignature;
	Luna::RootSignaturePtr m_terrainRootSignature;

	Luna::GraphicsPipelinePtr m_skyPipeline;
	Luna::GraphicsPipelinePtr m_terrainPipeline;
	bool m_pipelinesCreated{ false };

	SkyConstants m_skyConstants{};
	Luna::GpuBufferPtr m_skyConstantBuffer;

	TerrainConstants m_terrainConstants;
	Luna::GpuBufferPtr m_terrainConstantBuffer;

	Luna::GpuBufferPtr m_terrainIndices;
	Luna::GpuBufferPtr m_terrainVertices;

	Luna::ModelPtr m_skyModel;
	Luna::TexturePtr m_skyTexture;
	Luna::TexturePtr m_terrainTextureArray;
	Luna::TexturePtr m_terrainHeightMap;
	Luna::SamplerPtr m_samplerLinearMirror;
	Luna::SamplerPtr m_samplerLinearWrap;

	Luna::ResourceSet m_skyResources;
	Luna::ResourceSet m_terrainResources;

	Luna::CameraController m_controller;
};