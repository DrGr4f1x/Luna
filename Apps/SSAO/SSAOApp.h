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


class SSAOApp : public Luna::Application
{
public:
	SSAOApp(uint32_t width, uint32_t height);

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

	void InitRenderTargets();
	void InitRootSignatures();
	void InitPipelines();
	void InitConstantBuffers();
	void InitNoiseTexture();

#if !APP_DYNAMIC_DESCRIPTORS
	void InitDescriptorSets();
#endif // !APP_DYNAMIC_DESCRIPTORS

	void LoadAssets();

	void UpdateConstantBuffers();

protected:
	static const uint32_t SSAO_KERNEL_SIZE = 64;
	static const uint32_t SSAO_NOISE_DIM = 8;

	// G-Buffer
	Luna::ColorBufferPtr m_positionBuffer;
	Luna::ColorBufferPtr m_normalBuffer;
	Luna::ColorBufferPtr m_albedoBuffer;
	Luna::DepthBufferPtr m_depthBuffer;

	// Render targets
	Luna::ColorBufferPtr m_ssaoRenderTarget;
	Luna::ColorBufferPtr m_ssaoBlurRenderTarget;

	// Root signatures
	Luna::RootSignaturePtr m_gbufferRootSignature;
	Luna::RootSignaturePtr m_compositionRootSignature;
	Luna::RootSignaturePtr m_ssaoRootSignature;
	Luna::RootSignaturePtr m_ssaoBlurRootSignature;

	// Pipelines
	Luna::GraphicsPipelinePtr m_gbufferPipeline;
	Luna::GraphicsPipelinePtr m_compositionPipeline;
	Luna::GraphicsPipelinePtr m_ssaoPipeline;
	Luna::GraphicsPipelinePtr m_ssaoBlurPipeline;
	bool m_pipelinesCreated{ false };

	// G-Buffer vertex
	struct Vertex
	{
		float position[3];
		float normal[3];
		float color[4];
		float uv[2];
	};

	// Constants
	struct SceneConstants
	{
		Math::Matrix4 projectionMatrix{ Math::kIdentity };
		Math::Matrix4 modelMatrix{ Math::kIdentity };
		Math::Matrix4 viewMatrix{ Math::kIdentity };
		float nearPlane = 0.1f;
		float farPlane = 64.0f;
	};

	struct CompositionConstants
	{
		Math::Matrix4 dummyMatrix{ Math::kIdentity };
		int ssao{ 1 };
		int ssaoOnly{ 0 };
		int ssaoBlur{ 1 };
	};

	struct SSAOKernelConstants
	{
		Math::Vector4 samples[SSAO_KERNEL_SIZE];
	};

	struct SSAOConstants
	{
		Math::Matrix4 projectionMatrix{ Math::kIdentity };
		float gbufferTexDim[2];
		float noiseTexDim[2];
		float invDepthRangeA{ 1.0f };
		float invDepthRangeB{ 0.0f };
		float linearizeDepthA{ 1.0f };
		float linearizeDepthB{ 1.0f };
	};

	SceneConstants m_sceneConstants{};
	CompositionConstants m_compositionConstants{};
	SSAOKernelConstants m_ssaoKernelConstants{};
	SSAOConstants m_ssaoConstants{};

	// Constant buffers
	Luna::GpuBufferPtr m_sceneConstantBuffer;
	Luna::GpuBufferPtr m_compositionConstantBuffer;
	Luna::GpuBufferPtr m_ssaoKernelConstantBuffer;
	Luna::GpuBufferPtr m_ssaoConstantBuffer;

#if !APP_DYNAMIC_DESCRIPTORS
	// Descriptor sets
	std::vector<Luna::DescriptorSetPtr> m_sceneDescriptorSets;
	Luna::DescriptorSetPtr m_ssaoDescriptorSet;
	Luna::DescriptorSetPtr m_ssaoBlurDescriptorSet;
	Luna::DescriptorSetPtr m_compositionDescriptorSet;
#endif // !APP_DYNAMIC_DESCRIPTORS

	// Noise texture
	Luna::TexturePtr m_noiseTexture;

	// Sponza model
	Luna::ModelPtr m_model;

	Luna::CameraController m_controller{ m_camera, Math::Vector3(Math::kYUnitVector) };
	const float m_nearPlane = 0.1f;
	const float m_farPlane = 64.0f;
};