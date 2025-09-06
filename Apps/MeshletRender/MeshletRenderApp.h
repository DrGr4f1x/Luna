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


class MeshletRenderApp : public Luna::Application
{
public:
	MeshletRenderApp(uint32_t width, uint32_t height);

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

protected:
	struct Constants
	{
		Math::Matrix4 modelMatrix{ Math::kIdentity };
		Math::Matrix4 modelViewMatrix{ Math::kIdentity };
		Math::Matrix4 modelViewProjectionMatrix{ Math::kIdentity };
		uint32_t drawMeshlets{ 1 };
	};

	Constants m_constants{};
	Luna::GpuBufferPtr m_constantBuffer;

	Luna::DescriptorSetPtr m_cbvDescriptorSet;
	std::vector<Luna::DescriptorSetPtr> m_srvDescriptorSets;

	Luna::RootSignaturePtr m_rootSignature;
	Luna::MeshletPipelinePtr m_meshletPipeline;
	bool m_pipelineCreated{ false };

	MeshletModel m_model;

	Luna::CameraController m_controller{ m_camera, Math::Vector3(Math::kYUnitVector) };
};