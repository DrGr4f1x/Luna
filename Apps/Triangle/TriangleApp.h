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
#include "Graphics\GpuBuffer.h"
#include "Graphics\PipelineState.h"
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
	void InitRootSignature();
	void InitPipelineState();

private:
	// Vertex layout used in this example
	struct Vertex
	{
		float position[3];
		float color[3];
	};

	// Vertex buffer and attributes
	Luna::GpuBufferHandle m_vertexBuffer;

	// Index buffer
	Luna::GpuBufferHandle m_indexBuffer;

	// Uniform buffer block object
	Luna::GpuBufferHandle m_constantBuffer;

	// Vertex shader constants
	struct
	{
		Math::Matrix4 viewProjectionMatrix;
		Math::Matrix4 modelMatrix;
	} m_vsConstants;

	 // Root signature
	Luna::RootSignatureHandle m_rootSignature;

	// Pipeline state
	Luna::GraphicsPipelineHandle m_graphicsPipeline;
};