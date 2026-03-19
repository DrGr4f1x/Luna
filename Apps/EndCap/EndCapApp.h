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
#include "EndCapGenerator.h"

class EndCapApp : public Luna::Application
{
public:
	EndCapApp(uint32_t width, uint32_t height);

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
	void InitDescriptorSet();

	void UpdateConstantBuffers();

	void LoadAssets();

	void OnModelChanged();

protected:
	struct Vertex
	{
		float position[3];
		float color[4];
		float normal[3];
	};

	struct Constants
	{
		Math::Matrix4 viewProjectionMatrix{ Math::kIdentity };
		Math::Matrix4 modelMatrix{ Math::kIdentity };
		Math::Matrix4 modelViewMatrix{ Math::kIdentity };
		Math::Vector4 lightPos{ 0.0f, -2.0f, 1.0f, 0.0f };
		Math::Vector4 modelColor{ 0.5f, 0.5f, 0.5f, 0.0f };
		Math::Vector4 clipPlane{ 0.0f, 0.0f, 0.0f, 0.0f };
		float alpha{ 1.0f };
	};

	Luna::RootSignaturePtr m_meshRootSignature;
	Luna::GraphicsPipelinePtr m_meshPipeline;

	bool m_pipelinesCreated{ false };

	// Main model constants
	Luna::GpuBufferPtr m_modelConstantBuffer;
	Constants m_modelConstants;

	// Plane model variables
	Luna::GraphicsPipelinePtr m_planePipeline;
	Math::Matrix4 m_planeScaleMatrix;
	Luna::GpuBufferPtr m_planeConstantBuffer;
	Constants m_planeConstants;

	// End cap generator
	EndCapGenerator m_endCapGenerator;

	std::vector<Luna::ModelPtr> m_models;
	std::vector<std::string> m_modelNames;
	int32_t	m_curModel{ 0 };

	Luna::ModelPtr m_planeModel;

	Luna::CameraController m_controller{ m_camera, Math::Vector3(Math::kYUnitVector) };

	// End cap rendering
	Luna::RootSignaturePtr m_endCapRootSig;
	Luna::GraphicsPipelinePtr m_endCapPipeline;

	struct EndCapConstants
	{
		Math::Matrix4 viewProjectionMatrix{ Math::kIdentity };
		Math::Matrix4 modelMatrix{ Math::kIdentity };
		Math::Matrix4 modelViewMatrix{ Math::kIdentity };
		Math::Vector4 lightPos{ 0.0f, -2.0f, 1.0f, 0.0f };
		Math::Vector4 modelColor{ 0.5f, 0.5f, 0.5f, 0.0f };
	};

	Luna::GpuBufferPtr m_endCapConstantBuffer;
	EndCapConstants m_endCapConstants;

	Luna::DescriptorSetPtr m_endCapDescriptors;

	// Controls and position of the cut plane
	float m_planeDelta = 0.0f;
	float m_planeY = 0.0f;
	float m_minY = -1.0f;
	float m_maxY = 1.0f;
	float m_alpha{ 0.5f };

	// Scene controls
	bool m_applyCut{ false };
	bool m_multipleModels{ false };
	bool m_debugNormals{ false };
	bool m_showEndCap{ true };
	float m_normalLength{ 0.15f };
};