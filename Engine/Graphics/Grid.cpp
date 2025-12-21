//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#include "Stdafx.h"

#include "Grid.h"

#include "Application.h"
#include "Camera.h"
#include "CommandContext.h"
#include "CommonStates.h"

using namespace std;


namespace Luna
{

void Grid::Update(const Camera& camera)
{
	m_vsConstants.viewProjectionMatrix = camera.GetViewProjectionMatrix();
	m_constantBuffer->Update(sizeof(m_vsConstants), &m_vsConstants);
}


void Grid::Render(GraphicsContext& context)
{
	ScopedDrawEvent event(context, "Grid");

	context.SetRootSignature(m_rootSignature);
	context.SetGraphicsPipeline(m_pipeline);

	context.SetRootCBV(0, m_constantBuffer);

	context.SetVertexBuffer(0, m_vertexBuffer);
	context.SetIndexBuffer(m_indexBuffer);

	context.DrawIndexed((uint32_t)m_indexBuffer->GetElementCount());
}


void Grid::CreateDeviceDependentResources()
{
	InitMesh();
	InitRootSignature();

	// Create constant buffer
	m_constantBuffer = CreateConstantBuffer("Grid Constant Buffer", 1, sizeof(m_vsConstants));
	m_vsConstants.viewProjectionMatrix = Math::Matrix4(Math::kIdentity);
}


void Grid::CreateWindowSizeDependentResources()
{
	if (!m_pipelineCreated)
	{
		InitPipeline();
		m_pipelineCreated = true;
	}
}


void Grid::InitMesh()
{
	vector<Vertex> vertices;

	auto InsertVertex = [&vertices](float x, float y, float z, const Color& c)
		{
			vertices.push_back({ {x, y, z}, {c.R(), c.G(), c.B(), c.A()}});
		};

	const float width = float(m_width);
	const float height = float(m_height);

	// Horizontal lines
	float zCur = -10.0f;
	for (int j = -m_height; j <= m_height; ++j)
	{
		if (j == 0)
		{
			Color color = DirectX::Colors::WhiteSmoke;

			InsertVertex(-width, 0.0f, zCur, color);
			InsertVertex(0.0f, 0.0f, zCur, color);

			color = DirectX::Colors::Red;

			InsertVertex(0.0f, 0.0f, zCur, color);
			InsertVertex(width + 1.0f, 0.0f, zCur, color);
		}
		else
		{
			Color color = DirectX::Colors::WhiteSmoke;

			InsertVertex(-width, 0.0f, zCur, color);
			InsertVertex(width, 0.0f, zCur, color);
		}
		zCur += 1.0f;
	}

	// Vertical lines
	float xCur = -10.0f;
	for (int j = -m_width; j <= m_width; ++j)
	{
		if (j == 0)
		{
			Color color = DirectX::Colors::WhiteSmoke;

			InsertVertex(xCur, 0.0f, -height, color);
			InsertVertex(xCur, 0.0f, 0.0f, color);

			color = DirectX::Colors::Blue;

			InsertVertex(xCur, 0.0f, 0.0f, color);
			InsertVertex(xCur, 0.0f, height + 1.0f, color);
		}
		else
		{
			Color color = DirectX::Colors::WhiteSmoke;

			InsertVertex(xCur, 0.0f, -height, color);
			InsertVertex(xCur, 0.0f, height, color);
		}
		xCur += 1.0f;
	}

	Color color = DirectX::Colors::Green;
	InsertVertex(0.0f, 0.0f, 0.0f, color);
	InsertVertex(0.0f, height, 0.0f, color);

	m_vertexBuffer = CreateVertexBuffer<Vertex>("Grid Vertex Buffer", vertices);;

	vector<uint16_t> indices;
	for (size_t i = 0; i < vertices.size(); ++i)
	{
		indices.push_back(uint16_t(i));
	}

	m_indexBuffer = CreateIndexBuffer("Grid Index Buffer", indices);
}


void Grid::InitRootSignature()
{
	RootSignatureDesc desc{
		.name				= "Root Signature",
		.rootParameters		= {	RootCBV(0, ShaderStage::Vertex) }
	};

	m_rootSignature = m_application->CreateRootSignature(desc);
}


void Grid::InitPipeline()
{
	auto vertexLayout = VertexLayout<VertexComponent::PositionColor>();

	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= vertexLayout.GetSizeInBytes(),
		.inputClassification	= InputClassification::PerVertexData
	};

	GraphicsPipelineDesc pipelineDesc
	{
		.name				= "Grid Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadOnlyReversed(),
		.rasterizerState	= CommonStates::RasterizerTwoSided(),
		.rtvFormats			= { m_application->GetColorFormat() },
		.dsvFormat			= m_application->GetDepthFormat(),
		.topology			= PrimitiveTopology::LineList,
		.vertexShader		= { .shaderFile = "GridVS" },
		.pixelShader		= { .shaderFile = "GridPS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= vertexLayout.GetElements(),
		.rootSignature		= m_rootSignature
	};

	m_pipeline = m_application->CreateGraphicsPipeline(pipelineDesc);
}

} // namespace Luna