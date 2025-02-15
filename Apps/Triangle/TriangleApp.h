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
#include "Graphics\DepthBuffer.h"
#include "Graphics\GpuBuffer.h"
#include "Graphics\PipelineState.h"
#include "Graphics\ResourceSet.h"
#include "Graphics\RootSignature.h"


class TriangleApp final : public Luna::Application
{
public:
	TriangleApp(uint32_t width, uint32_t height);

	int ProcessCommandLine(int argc, char* argv[]) override;

	void Configure() override;
	void Startup() override;
	void Shutdown() override;

	void Update() override;
	void Render() override;

protected:
	void CreateDeviceDependentResources() override;
	void CreateWindowSizeDependentResources() override;

private:
	void InitDepthBuffer();
	void InitRootSignature();
	void InitPipelineState();
	void InitResources();

	void UpdateConstantBuffer();

private:
	// Camera controls
	float m_zoom{ -2.5f };
	Luna::CameraController m_controller;

	Luna::DepthBufferHandle m_depthBuffer;

	// Vertex layout used in this example
	struct Vertex
	{
		float position[3];
		float color[3];
	};

	// Vertex buffer and attributes
	Luna::GpuBuffer m_vertexBuffer;

	// Index buffer
	Luna::GpuBuffer m_indexBuffer;

	// Uniform buffer block object
	Luna::GpuBuffer m_constantBuffer;

	// Vertex shader constants
	struct
	{
		Math::Matrix4 viewProjectionMatrix;
		Math::Matrix4 modelMatrix;
	} m_vsConstants;

	 // Root signature
	Luna::RootSignature m_rootSignature;

	// Pipeline state
	Luna::GraphicsPipelineState m_graphicsPipeline;

	// Resources
	Luna::ResourceSet m_resources;
};