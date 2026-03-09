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

	void UpdateConstantBuffers();

	void LoadAssets();

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
	};

	Luna::RootSignaturePtr m_meshRootSignature;
	Luna::GraphicsPipelinePtr m_meshPipeline;

	bool m_pipelinesCreated{ false };

	// Main model constants
	Luna::GpuBufferPtr m_modelConstantBuffer;
	Constants m_modelConstants;

	// Plane model constants
	Luna::GpuBufferPtr m_planeConstantBuffer;
	Constants m_planeConstants;

	// End cap generator
	EndCapGenerator m_endCapGenerator;

	Luna::ModelPtr m_model;
	Math::BoundingBox m_modelBounds;

	Luna::ModelPtr m_planeModel;

	Luna::CameraController m_controller{ m_camera, Math::Vector3(Math::kYUnitVector) };

	// Controls and position of the cut plane
	float m_planeDelta = 0.0f;
	float m_planeY = 0.0f;
	float m_minY = -1.0f;
	float m_maxY = 1.0f;
};