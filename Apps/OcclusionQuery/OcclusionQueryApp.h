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

class OcclusionQueryApp : public Luna::Application
{
public:
	OcclusionQueryApp(uint32_t width, uint32_t height);

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
	void InitRootSignature();
	void InitPipelines();
	void InitConstantBuffers();
	void InitQueryHeap();
	void InitResourceSets();

	void UpdateConstantBuffers();

	void LoadAssets();

protected:
	struct VSConstants
	{
		Math::Matrix4 projectionMatrix{ Math::kIdentity };
		Math::Matrix4 modelViewMatrix{ Math::kIdentity };
		Math::Vector4 lightPos{ 10.0f, 10.0f, 10.0f, 1.0f };
		Luna::Color color{ DirectX::Colors::Black };
		float visible{ 1.0f };
	};

	VSConstants m_occluderConstants;
	VSConstants m_teapotConstants;
	VSConstants m_sphereConstants;

	Luna::DepthBufferPtr m_depthBuffer;

	Luna::GpuBufferPtr m_occluderConstantBuffer;
	Luna::GpuBufferPtr m_teapotConstantBuffer;
	Luna::GpuBufferPtr m_sphereConstantBuffer;

	Luna::ResourceSet m_occluderResources;
	Luna::ResourceSet m_teapotResources;
	Luna::ResourceSet m_sphereResources;

	Luna::RootSignaturePtr m_rootSignature;

	Luna::GraphicsPipelinePtr m_solidPipeline;
	Luna::GraphicsPipelinePtr m_simplePipeline;
	Luna::GraphicsPipelinePtr m_occluderPipeline;
	bool m_pipelinesCreated{ false };

	Luna::ModelPtr m_occluderModel;
	Luna::ModelPtr m_teapotModel;
	Luna::ModelPtr m_sphereModel;

	Luna::QueryHeapPtr m_queryHeap;
	Luna::GpuBufferPtr m_readbackBuffer;

	Luna::CameraController m_controller;
	float m_zoom{ -10.0f };

	uint32_t m_passedSamples[2]{ 0,0 };
};