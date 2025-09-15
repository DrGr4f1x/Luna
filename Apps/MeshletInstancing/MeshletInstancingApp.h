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

#include "MeshletModel.h"


class MeshletInstancingApp : public Luna::Application
{
public:
	MeshletInstancingApp(uint32_t width, uint32_t height);

	int ProcessCommandLine(int argc, char* argv[]) final;

	void Configure() final;
	void Startup() final;
	void Shutdown() final;

	void Update() final;
	void Render() final;

protected:
	void CreateDeviceDependentResources() final;
	void CreateWindowSizeDependentResources() final;

	void InitRootSignature();
	void InitPipeline();
	void InitDescriptorSets();

	void UpdateConstantBuffer();

	void RegenerateInstances();

protected:
	struct Constants
	{
		Math::Matrix4 viewMatrix{ Math::kIdentity };
		Math::Matrix4 viewProjectionMatrix{ Math::kIdentity };
		uint32_t drawMeshlets{ 1 };
	};

	Constants m_constants{};
	Luna::GpuBufferPtr m_constantBuffer;

	struct Instance
	{
		Math::Matrix4 worldMatrix{ Math::kIdentity };
		Math::Matrix4 worldInvTransposeMatrix{ Math::kIdentity };

	};

	Luna::GpuBufferPtr m_instanceBuffer;

	Luna::DescriptorSetPtr m_cbvDescriptorSet;
	std::vector<Luna::DescriptorSetPtr> m_srvDescriptorSets;
	Luna::DescriptorSetPtr m_instanceDescriptorSet;

	Luna::RootSignaturePtr m_rootSignature;
	Luna::MeshletPipelinePtr m_meshletPipeline;
	bool m_pipelineCreated{ false };

	MeshletModel m_model;

	int32_t m_instanceLevel{ 0 };
	const int32_t m_maxInstanceLevel{ 5 };
	uint32_t m_instanceCount{ 1 };

	Luna::CameraController m_controller{ m_camera, Math::Vector3(Math::kYUnitVector) };
};