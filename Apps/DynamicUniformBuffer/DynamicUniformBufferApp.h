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

class DynamicUniformBufferApp : public Luna::Application
{
public:
	DynamicUniformBufferApp(uint32_t width, uint32_t height);

	int ProcessCommandLine(int argc, char* argv[]) final;

	void Configure() final;
	void Startup() final;
	void Shutdown() final;

	void Update() final;
	void Render() final;

protected:
	void CreateDeviceDependentResources() final;
	void CreateWindowSizeDependentResources() final;

	void InitDepthBuffer();
	void InitRootSignature();
	void InitPipeline();
	void InitConstantBuffers();
	void InitBox();
	void InitResourceSet();

	void UpdateConstantBuffers();

protected:
	static const uint32_t m_numCubesSide{ 5 };
	static const uint32_t m_numCubes{ m_numCubesSide * m_numCubesSide * m_numCubesSide };

	struct VSConstants
	{
		Math::Matrix4 projectionMatrix;
		Math::Matrix4 viewMatrix;
	};
	VSConstants m_vsConstants;
	Luna::GpuBufferPtr m_vsConstantBuffer;

	struct alignas(256) VSModelConstants
	{
		Math::Matrix4* modelMatrix{ nullptr };
	};
	size_t m_dynamicAlignment{ 0 };
	VSModelConstants m_vsModelConstants;
	Luna::GpuBufferPtr m_vsModelConstantBuffer;

	Luna::DepthBufferPtr m_depthBuffer;

	Luna::RootSignaturePtr m_rootSignature;
	Luna::GraphicsPipelinePtr m_pipeline;
	bool m_pipelineCreated{ false };

	Luna::GpuBufferPtr m_vertexBuffer;
	Luna::GpuBufferPtr m_indexBuffer;

	Luna::ResourceSet m_resources;

	Luna::CameraController m_controller;

	float m_animationTimer{ 0.0f };
	Math::Vector3 m_rotations[m_numCubes];
	Math::Vector3 m_rotationSpeeds[m_numCubes];
};