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

#include "TriangleApp.h"

#include "Graphics\CommonStates.h"

using namespace Luna;
using namespace std;


TriangleApp::TriangleApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{
}


int TriangleApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void TriangleApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void TriangleApp::Startup()
{
	auto graphicsDevice = GetGraphicsDevice();

	// Setup vertices
	vector<Vertex> vertexData =
	{
		{ { -1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
		{ {  1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
		{ {  0.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
	};

	GpuBufferDesc vertexBufferDesc{
		.name			= "Vertex Buffer",
		.resourceType	= ResourceType::VertexBuffer,
		.memoryAccess	= MemoryAccess::GpuReadWrite,
		.elementCount	= vertexData.size(),
		.elementSize	= sizeof(Vertex),
		.initialData	= vertexData.data()
	};
	m_vertexBuffer = graphicsDevice->CreateGpuBuffer(vertexBufferDesc);

	// Setup indices
	vector<uint32_t> indexData = { 0, 1, 2 };
	GpuBufferDesc indexBufferDesc{
		.name			= "Index Buffer",
		.resourceType	= ResourceType::IndexBuffer,
		.memoryAccess	= MemoryAccess::GpuReadWrite,
		.elementCount	= indexData.size(),
		.elementSize	= sizeof(uint32_t),
		.initialData	= indexData.data()
	};
	m_indexBuffer = graphicsDevice->CreateGpuBuffer(indexBufferDesc);

	// Setup constant buffer
	GpuBufferDesc constantBufferDesc{
		.name			= "VS Constant Buffer",
		.resourceType	= ResourceType::ConstantBuffer,
		.memoryAccess	= MemoryAccess::GpuReadWrite,
		.elementCount	= 1,
		.elementSize	= sizeof(m_vsConstants),
		.initialData	= &m_vsConstants
	};
	m_constantBuffer = graphicsDevice->CreateGpuBuffer(constantBufferDesc);
	m_vsConstants.modelMatrix = Math::Matrix4(Math::kIdentity);

	InitRootSignature();
	InitPipelineState();
}


void TriangleApp::Shutdown()
{
	// Application cleanup on shutdown
}


void TriangleApp::Update()
{
	// Application update tick
	// Set m_bIsRunning to false if your application wants to exit
}


void TriangleApp::Render()
{
	// Application main render loop
	Application::Render();
}


void TriangleApp::CreateDeviceDependentResources()
{
	// Create any resources that depend on the device, but not the window size
}


void TriangleApp::CreateWindowSizeDependentResources()
{
	// Create any resources that depend on window size.  May be called when the window size changes.
}


void TriangleApp::InitRootSignature()
{
	auto rootSignatureDesc = RootSignatureDesc{
		.name				= "Root Sig",
		.flags				= RootSignatureFlags::AllowInputAssemblerInputLayout | RootSignatureFlags::DenyPixelShaderRootAccess,
		.rootParameters		= { RootParameter::RootCBV(0, ShaderStage::Vertex) }
	};

	m_rootSignature = GetGraphicsDevice()->CreateRootSignature(rootSignatureDesc);
}


void TriangleApp::InitPipelineState()
{
	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= sizeof(Vertex),
		.inputClassification	= InputClassification::PerVertexData
	};

	vector<VertexElementDesc> vertexElements{
		{ "POSITION", 0, Format::RGB32_Float, 0, offsetof(Vertex, position), InputClassification::PerVertexData, 0 },
		{ "COLOR", 0, Format::RGB32_Float, 0, offsetof(Vertex, color), InputClassification::PerVertexData, 0 }
	};

	GraphicsPipelineDesc desc
	{
		.name				= "Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadOnlyReversed(),
		.rasterizerState	= CommonStates::RasterizerTwoSided(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "TriangleVS" },
		.pixelShader		= { .shaderFile = "TrianglePS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= vertexElements,
		.rootSignature		= m_rootSignature.get()
	};

	m_graphicsPipeline = GetGraphicsDevice()->CreateGraphicsPipeline(desc);
}