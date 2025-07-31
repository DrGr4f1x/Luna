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

class GeometryShaderApp : public Luna::Application
{
public:
	GeometryShaderApp(uint32_t width, uint32_t height);

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

	void UpdateConstantBuffer();

	void LoadAssets();

protected:
	struct Vertex
	{
		float position[3];
		float normal[3];
		float color[4];
	};

	struct Constants
	{
		Math::Matrix4 projectionMatrix{ Math::kIdentity };
		Math::Matrix4 modelMatrix{ Math::kIdentity };
	};

	Luna::RootSignaturePtr m_meshRootSignature;
	Luna::RootSignaturePtr m_geomRootSignature;

	Luna::GraphicsPipelinePtr m_meshPipeline;
	Luna::GraphicsPipelinePtr m_geomPipeline;
	bool m_pipelinesCreated{ false };

	Constants m_constants{};
	Luna::GpuBufferPtr m_constantBuffer;

	Luna::DescriptorSetPtr m_vsCbvDescriptorSet;
	Luna::DescriptorSetPtr m_gsCbvDescriptorSet;

	Luna::ModelPtr m_model;

	Luna::CameraController m_controller{ m_camera, Math::Vector3(Math::kYUnitVector) };

	bool m_showNormals{ true };
};