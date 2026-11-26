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
#include "Graphics\RayTracing\RayTracingGeometry.h"
#include "Graphics\RayTracing\RayTracingPipeline.h"
#include "Graphics\RayTracing\ShaderBindingTable.h"


class RayQueryApp : public Luna::Application
{
public:
	RayQueryApp(uint32_t width, uint32_t height);

	int ProcessCommandLine(int argc, char* argv[]) final;

	void Configure() final;
	void Startup() final;
	void Shutdown() final;

	void Update() final;
	void Render() final;

protected:
	void CreateDeviceDependentResources() final;
	void CreateWindowSizeDependentResources() final;

	void LoadAssets();
	void InitRayTracingBLAS();
	void InitRayTracingTLAS();
	void InitRootSignature();
	void InitGraphicsPipeline();
	void InitDescriptorSet();

	void UpdateConstantBuffer();

protected:
	// Scene constants, constant buffer
	struct SceneConstants
	{
		Math::Matrix4 projection{ Math::kIdentity };
		Math::Matrix4 view{ Math::kIdentity };
		Math::Matrix4 model{ Math::kIdentity };
		Math::Vector3 lightPos{ Math::kZero };
	};
	SceneConstants m_sceneConstants{};
	Luna::GpuBufferPtr m_sceneConstantBuffer;

	// Root signature, graphics pipeline
	Luna::RootSignaturePtr m_rootSignature;
	Luna::GraphicsPipelinePtr m_graphicsPipeline;
	bool m_pipelineCreated{ false };

	// Geometry, acceleration structures
	Luna::ModelPtr m_model;
	std::vector<Luna::GpuBufferPtr> m_vertexBuffers;
	std::vector<Luna::GpuBufferPtr> m_indexBuffers;
	Luna::GpuBufferPtr m_blasBuffer;
	Luna::GpuBufferPtr m_tlasBuffer;

	// Descriptor set
	Luna::DescriptorSetPtr m_descriptorSet;

	// Camera controller
	Luna::CameraController m_controller{ m_camera, Math::Vector3(Math::kYUnitVector) };
};