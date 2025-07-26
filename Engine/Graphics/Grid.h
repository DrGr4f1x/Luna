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

#include "Graphics\GpuBuffer.h"
#include "Graphics\PipelineState.h"
#include "Graphics\ResourceSet.h"
#include "Graphics\RootSignature.h"


namespace Luna
{

// Forward declarations
class Application;
class Camera;
class GraphicsContext;


class Grid
{
public:
	explicit Grid(Application* application) 
		: m_application{ application } 
	{}

	void Update(const Camera& camera);
	void Render(GraphicsContext& context);

	void CreateDeviceDependentResources();
	void CreateWindowSizeDependentResources();

protected:
	void InitMesh();
	void InitRootSignature();
	void InitPipeline();
	void InitResourceSet();

protected:
	Application* m_application{ nullptr };

	int m_width{ 10 };
	int m_height{ 10 };
	float m_spacing{ 1.0f };

	struct Vertex
	{
		float position[3];
		float color[4];
	};

	// Vertex buffer and attributes
	Luna::GpuBufferPtr m_vertexBuffer;

	// Index buffer
	Luna::GpuBufferPtr m_indexBuffer;

	// Uniform buffer block object
	Luna::GpuBufferPtr m_constantBuffer;

	struct
	{
		Math::Matrix4 viewProjectionMatrix;
	} m_vsConstants;

	Luna::RootSignaturePtr m_rootSignature;
	Luna::GraphicsPipelinePtr m_pipeline;
	bool m_pipelineCreated{ false };

	Luna::ResourceSet m_resources;
};

} // namespace Luna