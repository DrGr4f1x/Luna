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

	void InitRootSignatures();
	void InitPipelines();
	void InitConstantBuffers();
	void InitCloth();
	void InitDescriptorSets();

	void LoadAssets();


	void UpdateConstantBuffers();
protected:
	struct VSConstants
	{
		Math::Matrix4 projectionMatrix{ Math::kIdentity };
		Math::Matrix4 modelViewMatrix{ Math::kIdentity };
		Math::Vector4 lightPos{ Math::kZero };
	};

	struct CSConstants
	{
		float deltaT{ 0.0f };
		float particleMass{ 0.0f };
		float springStiffness{ 0.0f };
		float damping{ 0.0f };
		float restDistH{ 0.0f };
		float restDistV{ 0.0f };
		float restDistD{ 0.0f };
		float sphereRadius{ 0.0f };
		Math::Vector4 spherePos{ Math::kIdentity };
		Math::Vector4 gravity{ Math::kIdentity };
		int32_t particleCount[2] = { 0, 0 };
		uint32_t calculateNormals{ 0 };
	};

	struct Particle
	{
		Math::Vector4 pos{ Math::kZero };
		Math::Vector4 vel{ Math::kZero };
		Math::Vector4 uv{ Math::kZero };
		Math::Vector4 normal{ Math::kZero };
		Math::Vector4 pinned{ Math::kZero };
	};

	Luna::RootSignaturePtr m_sphereRootSignature;
	Luna::RootSignaturePtr m_clothRootSignature;
	Luna::RootSignaturePtr m_computeRootSignature;

	Luna::GraphicsPipelinePtr m_spherePipeline;
	Luna::GraphicsPipelinePtr m_clothPipeline;
	Luna::ComputePipelinePtr m_computePipeline;
	bool m_pipelinesCreated{ false };

	VSConstants m_vsConstants{};
	Luna::GpuBufferPtr m_vsConstantBuffer;

	CSConstants m_csConstants{};
	Luna::GpuBufferPtr m_csConstantBuffer;
	Luna::GpuBufferPtr m_csNormalConstantBuffer;

	Luna::GpuBufferPtr m_clothBuffer[2];
	Luna::GpuBufferPtr m_clothIndexBuffer;

	Luna::ModelPtr m_sphereModel;
	Luna::TexturePtr m_texture;
	Luna::SamplerPtr m_sampler;

	Luna::DescriptorSetPtr m_sphereCbvDescriptorSet;
	Luna::DescriptorSetPtr m_clothCbvDescriptorSet;
	Luna::DescriptorSetPtr m_clothSrvDescriptorSet;
	Luna::DescriptorSetPtr m_samplerDescriptorSet;
	Luna::DescriptorSetPtr m_computeDescriptorSet[2];
	Luna::DescriptorSetPtr m_computeNormalDescriptorSet;

	Luna::CameraController m_controller;

	// Cloth dimensions
	const float m_sphereRadius{ 0.5f };
	const uint32_t m_gridSize[2]{ 64, 64 };
	const float m_size[2]{ 2.5f, 2.5f };
	bool m_simulateWind{ true };
	bool m_pinnedCloth{ false };
};