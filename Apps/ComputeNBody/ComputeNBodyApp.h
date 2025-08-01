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


class ComputeNBodyApp : public Luna::Application
{
public:
	ComputeNBodyApp(uint32_t width, uint32_t height);

	int ProcessCommandLine(int argc, char* argv[]) final;

	void Configure() final;
	void Startup() final;
	void Shutdown() final;

	void Update() final;
	void Render() final;

protected:
	void CreateDeviceDependentResources() final;
	void CreateWindowSizeDependentResources() final;

	void InitRootSignatures();
	void InitPipelines();
	void InitDescriptorSets();
	void InitParticles();

	void LoadAssets();

	void UpdateConstantBuffers();

protected:
	struct GraphicsConstants
	{
		Math::Matrix4 projectionMatrix{ Math::kIdentity };
		Math::Matrix4 modelViewMatrix{ Math::kIdentity };
		Math::Matrix4 invViewMatrix{ Math::kIdentity };
		float screenDim[2] = { 0.0f, 0.0f };
	};

	struct ComputeConstants
	{
		float deltaT{ 0.0f };
		float destX{ 0.0f };
		float destY{ 0.0f };
		int particleCount{ 0 };
	};

	struct Particle
	{
		Math::Vector4 pos{ Math::kZero };
		Math::Vector4 vel{ Math::kZero };
	};

	Luna::RootSignaturePtr m_rootSignature;
	Luna::RootSignaturePtr m_computeRootSignature;

	Luna::GraphicsPipelinePtr m_graphicsPipeline;
	Luna::ComputePipelinePtr m_computeCalculatePipeline;
	Luna::ComputePipelinePtr m_computeIntegratePipeline;
	bool m_pipelinesCreated{ false };

	GraphicsConstants m_graphicsConstants{};
	ComputeConstants m_computeConstants{};

	Luna::GpuBufferPtr m_graphicsConstantBuffer;
	Luna::GpuBufferPtr m_computeConstantBuffer;

	Luna::GpuBufferPtr m_particleBuffer;

	Luna::DescriptorSetPtr m_graphicsVsCbvSrvDescriptorSet;
	Luna::DescriptorSetPtr m_graphicsGsCbvDescriptorSet;
	Luna::DescriptorSetPtr m_graphicsPsSrvDescriptorSet;
	Luna::DescriptorSetPtr m_computeCbvUavDescriptorSet;

	Luna::TexturePtr m_gradientTexture;
	Luna::TexturePtr m_colorTexture;

	Luna::CameraController m_controller;
};