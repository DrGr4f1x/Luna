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


class ComputeClothApp : public Luna::Application
{
public:
	ComputeClothApp(uint32_t width, uint32_t height);

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

	void InitDepthBuffer();
	void InitRootSignatures();
	void InitPipelines();
	void InitConstantBuffers();
	void InitCloth();
	void InitResourceSets();

	void LoadAssets();


	void UpdateConstantBuffers();
protected:
	struct VSConstants
	{
		Math::Matrix4 projectionMatrix;
		Math::Matrix4 modelViewMatrix;
		Math::Vector4 lightPos;
	};

	struct CSConstants
	{
		float deltaT;
		float particleMass;
		float springStiffness;
		float damping;
		float restDistH;
		float restDistV;
		float restDistD;
		float sphereRadius;
		Math::Vector4 spherePos;
		Math::Vector4 gravity;
		int32_t particleCount[2];
		uint32_t calculateNormals;
	};

	struct Particle
	{
		Math::Vector4 pos;
		Math::Vector4 vel;
		Math::Vector4 uv;
		Math::Vector4 normal;
		Math::Vector4 pinned;
	};

	Luna::DepthBufferPtr m_depthBuffer;
	Luna::RootSignaturePtr m_sphereRootSignature;
	Luna::RootSignaturePtr m_clothRootSignature;
	Luna::RootSignaturePtr m_computeRootSignature;

	Luna::GraphicsPipelinePtr m_spherePipeline;
	Luna::GraphicsPipelinePtr m_clothPipeline;
	Luna::ComputePipelinePtr m_computePipeline;
	bool m_pipelinesCreated{ false };

	VSConstants m_vsConstants;
	Luna::GpuBufferPtr m_vsConstantBuffer;

	CSConstants m_csConstants;
	Luna::GpuBufferPtr m_csConstantBuffer;
	Luna::GpuBufferPtr m_csNormalConstantBuffer;

	Luna::GpuBufferPtr m_clothBuffer[2];
	Luna::GpuBufferPtr m_clothIndexBuffer;

	Luna::ModelPtr m_sphereModel;
	Luna::TexturePtr m_texture;
	Luna::SamplerPtr m_sampler;

	Luna::ResourceSet m_sphereResources;
	Luna::ResourceSet m_clothResources;
	Luna::ResourceSet m_computeResources[2];
	Luna::ResourceSet m_computeNormalResources;

	Luna::CameraController m_controller;

	// Cloth dimensions
	const float m_sphereRadius{ 0.5f };
	const uint32_t m_gridSize[2]{ 64, 64 };
	const float m_size[2]{ 2.5f, 2.5f };
	bool m_simulateWind{ true };
	bool m_pinnedCloth{ false };
};