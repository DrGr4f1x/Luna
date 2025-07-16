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
#include "Device.h"
#include "DeviceManager.h"

using namespace std;


namespace Luna
{

void Grid::Update(const Camera& camera)
{
	m_vsConstants.viewProjectionMatrix = camera.GetViewProjMatrix();
	m_constantBuffer->Update(sizeof(m_vsConstants), &m_vsConstants);
}


void Grid::Render(GraphicsContext& context)
{
	context.BeginEvent("Grid");

	context.SetRootSignature(m_rootSignature);
	context.SetGraphicsPipeline(m_pipeline);

	context.SetResources(m_resources);

	context.SetVertexBuffer(0, m_vertexBuffer);
	context.SetIndexBuffer(m_indexBuffer);

	context.DrawIndexed((uint32_t)m_indexBuffer->GetElementCount());

	context.EndEvent();
}


void Grid::CreateDeviceDependentResources()
{
	InitMesh();
	InitRootSignature();

	// Setup constant buffer
	GpuBufferDesc desc{
		.name			= "Grid Constant Buffer",
		.resourceType	= ResourceType::ConstantBuffer,
		.memoryAccess	= MemoryAccess::GpuRead | MemoryAccess::CpuWrite,
		.elementCount	= 1,
		.elementSize	= sizeof(m_vsConstants)
	};

	m_constantBuffer = GetDeviceManager()->GetDevice()->CreateGpuBuffer(desc);
	m_vsConstants.viewProjectionMatrix = Math::Matrix4(Math::kIdentity);

	InitResourceSet();
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

	GpuBufferDesc vertexBufferDesc{
		.name			= "Grid Vertex Buffer",
		.resourceType	= ResourceType::VertexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= vertices.size(),
		.elementSize	= sizeof(Vertex),
		.initialData	= vertices.data()
	};
	m_vertexBuffer = GetDeviceManager()->GetDevice()->CreateGpuBuffer(vertexBufferDesc);

	vector<uint16_t> indices;
	for (size_t i = 0; i < vertices.size(); ++i)
	{
		indices.push_back(uint16_t(i));
	}

	GpuBufferDesc indexBufferDesc{
		.name			= "Grid Index Buffer",
		.resourceType	= ResourceType::IndexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= indices.size(),
		.elementSize	= sizeof(uint16_t),
		.initialData	= indices.data()
	};
	m_indexBuffer = GetDeviceManager()->GetDevice()->CreateGpuBuffer(indexBufferDesc);
}


void Grid::InitRootSignature()
{
	auto desc = RootSignatureDesc{
		.name				= "Root Signature",
		.flags				= RootSignatureFlags::AllowInputAssemblerInputLayout,
		.rootParameters		= {	RootParameter::RootCBV(0, ShaderStage::Vertex) }
	};

	m_rootSignature = GetDeviceManager()->GetDevice()->CreateRootSignature(desc);
}


void Grid::InitPipeline()
{
	auto vertexLayout = VertexLayout<VertexComponent::PositionColor>();

	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= vertexLayout.GetSizeInBytes(),
		.inputClassification	= InputClassification::PerVertexData
	};

	auto deviceManager = GetDeviceManager();

	GraphicsPipelineDesc pipelineDesc
	{
		.name				= "Grid Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadOnlyReversed(),
		.rasterizerState	= CommonStates::RasterizerTwoSided(),
		.rtvFormats			= { deviceManager->GetColorFormat() },
		.dsvFormat			= deviceManager->GetDepthFormat(),
		.topology			= PrimitiveTopology::LineList,
		.vertexShader		= { .shaderFile = "GridVS" },
		.pixelShader		= { .shaderFile = "GridPS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= vertexLayout.GetElements(),
		.rootSignature		= m_rootSignature
	};

	m_pipeline = deviceManager->GetDevice()->CreateGraphicsPipeline(pipelineDesc);
}


void Grid::InitResourceSet()
{
	m_resources.Initialize(m_rootSignature);
	m_resources.SetCBV(0, 0, m_constantBuffer);
}

} // namespace Luna