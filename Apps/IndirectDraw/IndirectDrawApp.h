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


class IndirectDrawApp : public Luna::Application
{
public:
	IndirectDrawApp(uint32_t width, uint32_t height);

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
	void InitDescriptorSets();

	void UpdateConstantBuffer();

	void LoadAssets();

	void InitIndirectArgs();
	void InitInstanceData();

protected:
	// Root signatures
	Luna::RootSignaturePtr m_skySphereRootSignature;
	Luna::RootSignaturePtr m_groundRootSignature;
	Luna::RootSignaturePtr m_plantsRootSignature;

	// Graphics pipelines
	Luna::GraphicsPipelinePtr m_skySpherePipeline;
	Luna::GraphicsPipelinePtr m_groundPipeline;
	Luna::GraphicsPipelinePtr m_plantsPipeline;
	bool m_pipelinesCreated{false};

	// Constant buffer data
	struct VSConstants
	{
		Math::Matrix4 projectionMatrix{ Math::kIdentity};
		Math::Matrix4 modelViewMatrix{ Math::kIdentity };
	};

	// Constant buffer
	VSConstants m_vsConstants{};
	Luna::GpuBufferPtr m_vsConstantBuffer;

	// Instance buffer data
	struct InstanceData
	{
		float position[3];
		float rotation[3];
		float scale{ 1.0f };
		int32_t texIndex{ 0 };
	};

	// Instance buffer
	Luna::GpuBufferPtr m_instanceBuffer;

	// Indirect arguments buffer
	Luna::GpuBufferPtr m_indirectArgsBuffer;

	// Descriptor sets
	Luna::DescriptorSetPtr m_groundSrvDescriptorSet;
	Luna::DescriptorSetPtr m_plantsSrvDescriptorSet;

	// Assets
	Luna::ModelPtr m_skySphereModel;
	Luna::ModelPtr m_groundModel;
	Luna::ModelPtr m_plantsModel;
	Luna::TexturePtr m_plantsTextureArray;
	Luna::TexturePtr m_groundTexture;

	// Application state
	Luna::CameraController m_controller{ m_camera, Math::Vector3(Math::kYUnitVector) };
	uint32_t m_objectInstanceCount{ 2048 };
	float m_plantRadius{ 25.0f };
	uint32_t m_numObjects{ 0 };
};