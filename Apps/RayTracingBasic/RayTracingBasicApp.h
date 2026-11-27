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


class RayTracingBasicApp : public Luna::Application
{
public:
	RayTracingBasicApp(uint32_t width, uint32_t height);

	int ProcessCommandLine(int argc, char* argv[]) final;

	void Configure() final;
	void Startup() final;
	void Shutdown() final;

	void Update() final;
	void Render() final;

protected:
	void CreateDeviceDependentResources() final;
	void CreateWindowSizeDependentResources() final;

	void InitRayTracingBLAS();
	void InitRayTracingTLAS();
	void InitRootSignature();
	void InitRayTracingPipeline();
	void InitShaderBindingTable();
	void InitColorTarget();
	void InitDescriptorSet();

	void UpdateConstantBuffer();

protected:
	// Scene constants, constant buffer
	struct SceneConstants
	{
		Math::Matrix4 projectionToWorldMatrix{ Math::kIdentity };
		Math::Vector3 cameraPosition{ Math::kZero };
	};

	SceneConstants m_sceneConstants{};
	Luna::GpuBufferPtr m_sceneConstantBuffer;

	// Ray tracing BLAS, TLAS
	Luna::GpuBufferPtr m_blasBuffer;
	Luna::GpuBufferPtr m_tlasBuffer;;

	// Root signature, ray tracing pipeline, shader binding table
	Luna::RootSignaturePtr m_rootSignature;
	Luna::RayTracingPipelinePtr m_rayTracingPipeline;
	Luna::ShaderBindingTablePtr m_shaderBindingTable;

	// Ray tracing render target, descriptor set
	Luna::ColorBufferPtr m_colorTarget;
	Luna::DescriptorSetPtr m_descriptorSet;
};