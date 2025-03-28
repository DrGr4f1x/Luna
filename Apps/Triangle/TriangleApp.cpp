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

#include "Graphics\ColorBuffer.h"
#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"

using namespace Luna;
using namespace std;


TriangleApp::TriangleApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
	, m_controller{ m_camera, Math::Vector3(Math::kYUnitVector) }
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
	m_vertexBuffer.Initialize(vertexBufferDesc);

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
	m_indexBuffer.Initialize(indexBufferDesc);

	// Setup constant buffer
	GpuBufferDesc constantBufferDesc{
		.name			= "VS Constant Buffer",
		.resourceType	= ResourceType::ConstantBuffer,
		.memoryAccess	= MemoryAccess::GpuRead | MemoryAccess::CpuWrite,
		.elementCount	= 1,
		.elementSize	= sizeof(m_vsConstants),
		.initialData	= nullptr
	};
	m_constantBuffer.Initialize(constantBufferDesc);
	m_vsConstants.modelMatrix = Math::Matrix4(Math::kIdentity);

	// Setup camera
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		256.0f);
	m_camera.SetPosition(Math::Vector3(0.0f, 0.0f, -m_zoom));
	m_camera.Update();

	m_controller.SetSpeedScale(0.025f);
	m_controller.RefreshFromCamera();

	UpdateConstantBuffer();

	InitDepthBuffer();
	InitRootSignature();
	InitPipelineState();
	InitResources();
}


void TriangleApp::Shutdown()
{
	// Application cleanup on shutdown
}


void TriangleApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds());
}


void TriangleApp::Render()
{
	ScopedEvent event{ "TriangleApp::Render" };

	auto& context = GraphicsContext::Begin("Frame");

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	Color clearColor{ DirectX::Colors::CornflowerBlue };
	context.ClearColor(GetColorBuffer(), clearColor);

	context.TransitionResource(m_depthBuffer, ResourceState::DepthWrite);
	context.ClearDepth(m_depthBuffer);

	context.BeginRendering(GetColorBuffer(), m_depthBuffer);

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());

	context.SetRootSignature(m_rootSignature);
	context.SetGraphicsPipeline(m_graphicsPipeline);

	context.SetResources(m_resources);
	context.SetConstantBuffer(0, m_constantBuffer);

	context.SetPrimitiveTopology(PrimitiveTopology::TriangleList);

	//context.SetCBV(0, 0, m_constantBuffer);

	/*vector<Vertex> vertexData =
	{
		{ { -1.0f, -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
		{ {  1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
		{ {  0.0f,  1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
	};
	context.SetDynamicVB(0, 3, sizeof(Vertex), vertexData.data());*/

	context.SetVertexBuffer(0, m_vertexBuffer);
	
	context.SetIndexBuffer(m_indexBuffer);

	context.DrawIndexed((uint32_t)m_indexBuffer.GetElementCount());

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void TriangleApp::CreateDeviceDependentResources()
{
	// Create any resources that depend on the device, but not the window size
}


void TriangleApp::CreateWindowSizeDependentResources()
{
	// Create any resources that depend on window size.  May be called when the window size changes.
}


void TriangleApp::InitDepthBuffer()
{
	DepthBufferDesc depthBufferDesc{
		.name			= "Depth Buffer",
		.resourceType	= ResourceType::Texture2D,
		.width			= GetWindowWidth(),
		.height			= GetWindowHeight(),
		.format			= GetDepthFormat()
	};

	m_depthBuffer.Initialize(depthBufferDesc);
}


void TriangleApp::InitRootSignature()
{
	auto rootSignatureDesc = RootSignatureDesc{
		.name				= "Root Sig",
		.flags				= RootSignatureFlags::AllowInputAssemblerInputLayout | RootSignatureFlags::DenyPixelShaderRootAccess,
		.rootParameters		= { RootParameter::RootCBV(0, ShaderStage::Vertex) }
	};

	m_rootSignature.Initialize(rootSignatureDesc);
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
		.rootSignature		= m_rootSignature.GetHandle()
	};

	m_graphicsPipeline.Initialize(desc);
}


void TriangleApp::InitResources()
{
	m_resources.Initialize(m_rootSignature);
	m_resources.SetCBV(0, 0, m_constantBuffer);
}


void TriangleApp::UpdateConstantBuffer()
{
	// Update matrices
	m_vsConstants.viewProjectionMatrix = m_camera.GetViewProjMatrix();

	m_constantBuffer.Update(sizeof(m_vsConstants), &m_vsConstants);
}