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


class PipelineStatisticsApp : public Luna::Application
{
public:
	PipelineStatisticsApp(uint32_t width, uint32_t height);

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

	void InitRootSignature();
	void InitPipeline();
	void InitQueryHeap();
	void InitResourceSet();

	void UpdateConstantBuffer();

	void LoadAssets();

protected:
	struct VSConstants
	{
		Math::Matrix4 projectionMatrix{ Math::kIdentity };
		Math::Matrix4 modelViewMatrix{ Math::kIdentity };
		Math::Vector4 lightPos{ 10.0f, 10.0f, 10.0f, 1.0f };
	};

	Luna::RootSignaturePtr m_rootSignature;

	Luna::GraphicsPipelinePtr m_pipeline;
	bool m_pipelinesCreated{ false };

	// Pipeline features
	int32_t m_cullMode{ 1 };
	bool m_blendingEnabled{ false };
	bool m_discardEnabled{ false };
	bool m_wireframeEnabled{ false };
	bool m_tessellationEnabled{ false };

	VSConstants m_vsConstants{};
	Luna::GpuBufferPtr m_vsConstantBuffer;

	Luna::QueryHeapPtr m_queryHeap;
	Luna::GpuBufferPtr m_readbackBuffer;

	Luna::PipelineStatistics m_statistics{};

	Luna::ResourceSet m_resourceSet;

	std::vector<Luna::ModelPtr> m_models;
	std::vector<std::string> m_modelNames;
	int32_t	m_curModel{ 0 };

	Luna::CameraController m_controller{ m_camera, Math::Vector3(Math::kYUnitVector) };
	float m_zoom{ -10.0f };

	// Application state
	int32_t m_gridSize{ 3 };
};