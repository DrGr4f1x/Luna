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

class RayTracingIntersectionApp : public Luna::Application
{
public:
	RayTracingIntersectionApp(uint32_t width, uint32_t height);

	int ProcessCommandLine(int argc, char* argv[]) final;

	void Configure() final;
	void Startup() final;
	void Shutdown() final;

	void Update() final;
	void Render() final;

protected:
	void CreateDeviceDependentResources() final;
	void CreateWindowSizeDependentResources() final;

	void InitGeometry();
	void InitRayTracingBLAS();
	void InitRayTracingTLAS();
	void InitRootSignature();
	void InitRayTracingPipeline();
	void InitShaderBindingTable();
	void InitColorTarget();
	void InitDescriptorSet();

	void UpdateConstantBuffer();

protected:
	struct SceneConstants
	{
		Math::Matrix4 projectionToWorld{ Math::kIdentity };
		Math::Vector3 cameraPos{ Math::kZero };
		Math::Vector4 lightPos{ Math::kZero };
	};

	SceneConstants m_sceneConstants{};
	Luna::GpuBufferPtr m_sceneConstantBuffer;

	// Spheres and AABBs
	struct Sphere
	{
		Math::Vector4 centerRadius{ Math::kZero };
		Math::Vector4 color{ Math::kZero };
	};
	struct AABB
	{
		DirectX::XMFLOAT3 min;
		DirectX::XMFLOAT3 max;
	};

	Luna::GpuBufferPtr m_sphereBuffer;
	Luna::GpuBufferPtr m_aabbBuffer;
	const uint32_t m_numSpheres = 1024;

	// Ray tracing acceleration structures
	Luna::GpuBufferPtr m_blasBuffer;
	Luna::GpuBufferPtr m_tlasBuffer;

	// Root signature, pipeline, SBT
	Luna::RootSignaturePtr m_globalRootSignature;
	Luna::RayTracingPipelinePtr m_rayTracingPipeline;
	Luna::ShaderBindingTablePtr m_shaderBindingTable;

	// Ray tracing render target, descriptor set
	Luna::ColorBufferPtr m_colorTarget;
	Luna::DescriptorSetPtr m_descriptorSet;

	 // Camera controller
	Luna::CameraController m_controller{ m_camera, Math::Vector3(Math::kYUnitVector) };
};