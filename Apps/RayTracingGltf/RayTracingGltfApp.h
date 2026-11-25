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

class RayTracingGltfApp : public Luna::Application
{
public:
	RayTracingGltfApp(uint32_t width, uint32_t height);

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

	// Root signature, pipeline, SBT
	Luna::RootSignaturePtr m_rootSignature;
	Luna::RayTracingPipelinePtr m_rayTracingPipeline;
	Luna::ShaderBindingTablePtr m_shaderBindingTable;

	// Geometry, acceleration structures
	Luna::ModelPtr m_model;
	std::vector<Luna::GpuBufferPtr> m_vertexBuffers;
	std::vector<Luna::GpuBufferPtr> m_indexBuffers;
	Luna::GpuBufferPtr m_blasBuffer;
	Luna::GpuBufferPtr m_tlasBuffer;

	// Scene constants, constant buffer
	struct SceneConstants
	{
		Math::Matrix4 viewInverse{ Math::kIdentity };
		Math::Matrix4 projInverse{ Math::kIdentity };
		Math::Matrix4 projectionToWorld{ Math::kIdentity };
		Math::Vector3 cameraPosition{ Math::kZero };
		uint32_t frame{ 0 };
	};
	SceneConstants m_sceneConstants{};
	Luna::GpuBufferPtr m_sceneConstantBuffer;

	// Textures, sampler
	std::vector<Luna::TexturePtr> m_diffuseTextures;
	std::vector<Luna::TexturePtr> m_occlusionTextures;
	Luna::SamplerPtr m_sampler;

	// Ray tracing render target, descriptor set
	Luna::ColorBufferPtr m_colorTarget;
	Luna::DescriptorSetPtr m_descriptorSet;
	Luna::DescriptorSetPtr m_bindlessVertexBufferSet;
	Luna::DescriptorSetPtr m_bindlessIndexBufferSet;
	Luna::DescriptorSetPtr m_bindlessDiffuseTextureSet;
	Luna::DescriptorSetPtr m_bindlessOcclusionTextureSet;
	Luna::DescriptorSetPtr m_samplerSet;

	// Camera controller
	Luna::CameraController m_controller{ m_camera, Math::Vector3(Math::kYUnitVector) };
};