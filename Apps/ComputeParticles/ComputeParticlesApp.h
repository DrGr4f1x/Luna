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


class ComputeParticlesApp : public Luna::Application
{
public:
	ComputeParticlesApp(uint32_t width, uint32_t height);

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
	void InitParticles();
	void InitResourceSets();

	void UpdateConstantBuffers();

	void LoadAssets();

protected:
	struct Particle
	{
		float pos[2];
		float vel[2];
		Math::Vector4 gradientPos;
	};

	const uint32_t m_particleCount{ 256 * 1024 };

	Luna::DepthBufferPtr m_depthBuffer;

	Luna::GpuBufferPtr m_particleBuffer;

	struct CSConstants
	{
		float deltaT;
		float destX;
		float destY;
		int particleCount;
	};

	CSConstants m_csConstants;
	Luna::GpuBufferPtr m_csConstantBuffer;

	struct VSConstants
	{
		float invTargetSize[2];
		float pointSize;
	};

	VSConstants m_vsConstants;
	Luna::GpuBufferPtr m_vsConstantBuffer;

	Luna::RootSignaturePtr m_graphicsRootSignature;
	Luna::RootSignaturePtr m_computeRootSignature;

	Luna::GraphicsPipelinePtr m_graphicsPipeline;
	Luna::ComputePipelinePtr m_computePipeline;
	bool m_pipelinesCreated{ false };

	Luna::TexturePtr m_colorTexture;
	Luna::TexturePtr m_gradientTexture;
	Luna::SamplerPtr m_sampler;

	Luna::ResourceSet		m_computeResources;
	Luna::ResourceSet		m_gfxResources;

	bool m_animate{ true };
	float m_animStart{ 20.0f };
	float m_localTimer{ 0.0f };
};