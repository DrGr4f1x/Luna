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


class OITApp : public Luna::Application
{
public:
	OITApp(uint32_t width, uint32_t height);

	int ProcessCommandLine(int argc, char* argv[]) final;

	void Configure() final;
	void Startup() final;
	void Shutdown() final;

	void Update() final;
	void Render() final;

protected:
	void CreateDeviceDependentResources() final;
	void CreateWindowSizeDependentResources() final;

	void InitRootSignatures();
	void InitPipelines();
	void InitLinkedListData();
	void InitDescriptorSets();
	void UpdateConstantBuffer();
	void LoadAssets();

protected:
	// Constant data
	struct SceneConstants
	{
		Math::Matrix4 viewMatrix{ Math::kIdentity };
		Math::Matrix4 projectionMatrix{ Math::kIdentity };
	};
	SceneConstants m_sceneConstants{};

	// Constant buffer
	Luna::GpuBufferPtr m_sceneConstantBuffer;

	// Root signatures and PSOs
	Luna::RootSignaturePtr m_clearBufferRootSignature;
	Luna::RootSignaturePtr m_clearColorRootSignature;
	Luna::RootSignaturePtr m_geometryRootSignature;
	Luna::RootSignaturePtr m_colorRootSignature;
	Luna::ComputePipelinePtr m_clearBufferPipeline;
	Luna::ComputePipelinePtr m_clearColorPipeline;
	Luna::GraphicsPipelinePtr m_geometryPipeline;
	Luna::GraphicsPipelinePtr m_colorPipeline;
	bool m_pipelinesCreated{ false };

	// Structured buffers and texture UAV
	struct GeometryInfo
	{
		uint32_t count{ 0 };
		uint32_t maxNodeCount{ 0 };
	};
	GeometryInfo m_geometryInfo{};

	struct Node
	{
		float color[4];
		float depth{ 0.0f };
		uint32_t next{ 0 };
	};

	const uint32_t m_nodeCount{ 20 };
	Luna::GpuBufferPtr m_geometryInfoBuffer;
	Luna::GpuBufferPtr m_linkedListBuffer;
	Luna::ColorBufferPtr m_listHeadTexture;

	// Descriptor sets
	Luna::DescriptorSetPtr m_clearColorDescriptorSet;
	Luna::DescriptorSetPtr m_geometryDescriptorSet;
	Luna::DescriptorSetPtr m_colorDescriptorSet;

	// Models
	struct ObjectData
	{
		Math::Matrix4 modelMatrix{ Math::kIdentity };
		Math::Vector4 color{ Math::kZero };
	};
	Luna::ModelPtr m_cubeModel;
	Luna::ModelPtr m_sphereModel;

	// Camera controller
	Luna::CameraController m_controller{ m_camera, Math::Vector3(Math::kYUnitVector) };
};