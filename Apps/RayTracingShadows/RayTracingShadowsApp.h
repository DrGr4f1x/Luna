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


class RayTracingShadowsApp : public Luna::Application
{
public:
	RayTracingShadowsApp(uint32_t width, uint32_t height);

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
	void InitRayTracingPipeline();
	void InitShaderBindingTable();
	void InitColorTarget();
	void InitDescriptorSets();

	void UpdateConstantBuffer();

protected:
	struct SceneConstants
	{
		Math::Matrix4 projectionToWorld{ Math::kZero };
		Math::Vector3 cameraPosition{ Math::kZero };
		Math::Vector3 lightPosition{ Math::kZero };
		int vertexStride{ 0 };
	};

	SceneConstants m_sceneConstants{};
	Luna::GpuBufferPtr m_sceneConstantBuffer;

	// Geometry and ray tracing acceleration structures
	Luna::ModelPtr m_model;
	//Luna::GpuBufferPtr m_blasBuffer;
	std::vector<Luna::GpuBufferPtr> m_vertexBuffers;
	std::vector<Luna::GpuBufferPtr> m_indexBuffers;
	std::vector<Luna::GpuBufferPtr> m_blasBuffers;
	Luna::GpuBufferPtr m_tlasBuffer;

	// Root signature, ray tracing pipeline, shader binding table
	Luna::RootSignaturePtr m_globalRootSignature;
	Luna::RayTracingPipelinePtr m_rayTracingPipeline;
	Luna::ShaderBindingTablePtr m_shaderBindingTable;

	// Ray tracing render target, descriptor set
	Luna::ColorBufferPtr m_colorTarget;
	Luna::DescriptorSetPtr m_descriptorSet;
	Luna::DescriptorSetPtr m_bindlessVertexBufferSet;
	Luna::DescriptorSetPtr m_bindlessIndexBufferSet;

	Luna::CameraController m_controller{ m_camera, Math::Vector3(Math::kYUnitVector) };
};