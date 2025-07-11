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

#include "PipelinesApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"

using namespace Luna;
using namespace Math;
using namespace std;


PipelinesApp::PipelinesApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
	, m_controller{ m_camera, Vector3(kYUnitVector) }
{}


int PipelinesApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void PipelinesApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void PipelinesApp::Startup()
{
	// Application initialization, after device creation
}


void PipelinesApp::Shutdown()
{
	// Application cleanup on shutdown
}


void PipelinesApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffer();
}


void PipelinesApp::UpdateUI()
{
	if (m_uiOverlay->Header("Camera"))
	{
		char str[256];
		auto position = m_camera.GetPosition();
		sprintf_s(str, "(%2.2f, %2.2f, %2.2f)", (double)position.GetX(), (double)position.GetY(), (double)position.GetZ());
		m_uiOverlay->Text(str);
	}
}


void PipelinesApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	auto colorBuffer = GetColorBuffer();
	context.TransitionResource(colorBuffer, ResourceState::RenderTarget);
	context.TransitionResource(m_depthBuffer, ResourceState::DepthWrite);
	context.ClearColor(colorBuffer);
	context.ClearDepthAndStencil(m_depthBuffer);

	context.BeginRendering(colorBuffer, m_depthBuffer);

	const uint32_t width = GetWindowWidth();
	const uint32_t height = GetWindowHeight();

	context.SetScissor(0, 0, width, height);

	context.SetRootSignature(m_rootSignature);
	context.SetResources(m_resources);

	// Left : solid color
	{
		ScopedDrawEvent event(context, "Left: solid color");
		context.SetViewport(0.0f, 0.0f, (float)width / 3.0f, (float)height);
		context.SetGraphicsPipeline(m_phongPipeline);
		m_model->Render(context);
	}

	// Middle : toon shading
	{
		ScopedDrawEvent event(context, "Middle: toon shading");
		context.SetViewport((float)width / 3.0f, 0.0f, (float)width / 3.0f, (float)height);
		context.SetGraphicsPipeline(m_toonPipeline);
		m_model->Render(context);
	}

	// Right : wireframe
	{
		ScopedDrawEvent event(context, "Right: wireframe");
		context.SetViewport(2.0f * (float)width / 3.0f, 0, (float)width / 3.0f, (float)height);
		context.SetGraphicsPipeline(m_wireframePipeline);
		m_model->Render(context);
	}

	//RenderGrid(context);
	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(colorBuffer, ResourceState::Present);

	context.Finish();
}


void PipelinesApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		(float)GetWindowHeight() / (float)(GetWindowHeight() / 3),
		0.1f,
		512.0f);
	m_camera.SetPosition(Vector3(2.7f, -6.3f, -8.75f));
	m_camera.Update();

	InitRootSignature();
	InitConstantBuffer();
	LoadAssets();
	InitResourceSet();

	auto box = m_model->boundingBox;

	m_controller.SetSpeedScale(0.01f);
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.SetOrbitTarget(box.GetCenter(), Length(m_camera.GetPosition()), 0.25f);
}


void PipelinesApp::CreateWindowSizeDependentResources()
{
	InitDepthBuffer();
	if (!m_pipelinesCreated)
	{
		InitPipelines();
		m_pipelinesCreated = true;
	}

	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		(float)GetWindowHeight() / (float)(GetWindowHeight() / 3),
		0.1f,
		512.0f);
	m_camera.Update();
}


void PipelinesApp::InitDepthBuffer()
{
	DepthBufferDesc depthBufferDesc{
		.name	= "Depth Buffer",
		.width	= GetWindowWidth(),
		.height = GetWindowHeight(),
		.format = GetDepthFormat()
	};

	m_depthBuffer = CreateDepthBuffer(depthBufferDesc);
}


void PipelinesApp::InitRootSignature()
{
	auto rootSignatureDesc = RootSignatureDesc{
		.name = "Root Signature",
		.flags = RootSignatureFlags::AllowInputAssemblerInputLayout,
		.rootParameters = {	RootParameter::RootCBV(0, ShaderStage::Vertex) }
	};

	m_rootSignature = CreateRootSignature(rootSignatureDesc);
}


void PipelinesApp::InitPipelines()
{
	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= sizeof(Vertex),
		.inputClassification	= InputClassification::PerVertexData
	};

	auto layout = VertexLayout<VertexComponent::PositionNormalColorTexcoord>();

	// Phong pipeline
	GraphicsPipelineDesc phongPipelineDesc
	{
		.name				= "Phong Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState	= CommonStates::RasterizerDefault(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "PhongVS" },
		.pixelShader		= { .shaderFile = "PhongPS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= layout.GetElements(),
		.rootSignature		= m_rootSignature
	};

	m_phongPipeline = CreateGraphicsPipelineState(phongPipelineDesc);

	// Toon pipeline
	GraphicsPipelineDesc toonPipelineDesc = phongPipelineDesc;
	toonPipelineDesc.SetName("Toon Graphics PSO");
	toonPipelineDesc.SetVertexShader("ToonVS");
	toonPipelineDesc.SetPixelShader("ToonPS");

	m_toonPipeline = CreateGraphicsPipelineState(toonPipelineDesc);

	// Wireframe pipeline
	GraphicsPipelineDesc wireframePipelineDesc = phongPipelineDesc;
	wireframePipelineDesc.SetName("Wireframe Graphics PSO");
	wireframePipelineDesc.SetRasterizerState(CommonStates::RasterizerWireframe());
	wireframePipelineDesc.SetVertexShader("WireframeVS");
	wireframePipelineDesc.SetPixelShader("WireframePS");

	m_wireframePipeline = CreateGraphicsPipelineState(wireframePipelineDesc);
}


void PipelinesApp::InitConstantBuffer()
{
	GpuBufferDesc desc{
		.name			= "Constant Buffer",
		.resourceType	= ResourceType::ConstantBuffer,
		.memoryAccess	= MemoryAccess::GpuRead | MemoryAccess::CpuWrite,
		.elementCount	= 1,
		.elementSize	= sizeof(VSConstants)
	};
	m_constantBuffer = CreateGpuBuffer(desc);
}


void PipelinesApp::InitResourceSet()
{
	m_resources.Initialize(m_rootSignature);
	m_resources.SetCBV(0, 0, m_constantBuffer);
}


void PipelinesApp::UpdateConstantBuffer()
{
	m_vsConstants.projectionMatrix = m_camera.GetProjMatrix();
	m_vsConstants.modelMatrix = m_camera.GetViewMatrix();
	m_vsConstants.lightPos = Vector4(0.0f, 2.0f, -1.0f, 0.0f);

	m_constantBuffer->Update(sizeof(m_vsConstants), &m_vsConstants);
}


void PipelinesApp::LoadAssets()
{
	auto layout = VertexLayout<VertexComponent::PositionNormalColorTexcoord>();

	m_model = LoadModel("treasure_smooth.gltf", layout, 1.0f);
}