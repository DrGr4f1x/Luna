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
	Luna::GpuBufferPtr m_indexBuffer;
	Luna::GpuBufferPtr m_vertexBuffer;

	Luna::GpuBufferPtr m_triangleBLAS;
	Luna::GpuBufferPtr m_rayTracingTLAS;

	Luna::RootSignaturePtr m_globalRootSignature;

	Luna::RayTracingPipelinePtr m_rayTracingPipeline;
	Luna::ShaderBindingTablePtr m_shaderBindingTable;

	struct RayGenConstants
	{
		Math::Matrix4 viewInverseMatrix{ Math::kIdentity };
		Math::Matrix4 projectionInverseMatrix{ Math::kIdentity };
	};

	RayGenConstants m_rayGenConstants{};
	Luna::GpuBufferPtr m_rayGenConstantBuffer;

	Luna::ColorBufferPtr m_colorTarget;
	Luna::DescriptorSetPtr m_testSet;
	Luna::DescriptorSetPtr m_descriptorSet;
};