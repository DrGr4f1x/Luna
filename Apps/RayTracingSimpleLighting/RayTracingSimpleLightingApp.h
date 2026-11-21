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
#include "Graphics\RayTracing\RayTracingGeometry.h"
#include "Graphics\RayTracing\RayTracingPipeline.h"
#include "Graphics\RayTracing\ShaderBindingTable.h"


class RayTracingSimpleLightingApp : public Luna::Application
{
public:
	RayTracingSimpleLightingApp(uint32_t width, uint32_t height);

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
	void InitRootSignatures();
	void InitRayTracingPipeline();
	void InitShaderBindingTable();
	void InitColorTarget();
	void InitDescriptorSets();

	void UpdateCameraMatrices();
	void UpdateConstantBuffer();

protected:
	struct SceneConstants
	{
		Math::Matrix4 projectionToWorld{ Math::kIdentity };
		Math::Vector4 cameraPosition{ Math::kZero };
		Math::Vector4 lightPosition{ Math::kZero };
		Math::Vector4 lightAmbientColor{ Math::kZero };
		Math::Vector4 lightDiffuseColor{ Math::kZero };
	};
	SceneConstants m_sceneConstants;

	struct CubeConstants
	{
		Math::Vector4 albedo{ Math::kIdentity };
	};
	CubeConstants m_cubeConstants;

	Luna::GpuBufferPtr m_sceneConstantBuffer;

	Luna::RootSignaturePtr m_globalRootSignature;
	Luna::RootSignaturePtr m_localRootSignature;

	Luna::RayTracingPipelinePtr m_rayTracingPipeline;
	Luna::ShaderBindingTablePtr m_shaderBindingTable;

	struct Vertex
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 normal;
	};

	Luna::GpuBufferPtr m_indexBuffer;
	Luna::GpuBufferPtr m_vertexBuffer;

	Luna::GpuBufferPtr m_cubeBLAS;
	Luna::GpuBufferPtr m_rayTracingTLAS;

	Luna::ColorBufferPtr m_colorTarget;
	
	Luna::DescriptorSetPtr m_colorTargetDescriptorSet;
	Luna::DescriptorSetPtr m_geometryBufferDescriptorSet;

	// Camera
	Math::Vector4 m_eye{ Math::kIdentity };
	Math::Vector4 m_at{ Math::kIdentity };
	Math::Vector4 m_up{ Math::kIdentity };
};