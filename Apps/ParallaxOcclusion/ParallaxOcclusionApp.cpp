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

#include "ParallaxOcclusionApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"

using namespace Luna;
using namespace Math;
using namespace std;


ParallaxOcclusionApp::ParallaxOcclusionApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{
}


int ParallaxOcclusionApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void ParallaxOcclusionApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
	m_uiWidth = 250.0f;
}


void ParallaxOcclusionApp::Startup()
{
	// Application initialization, after device creation
}


void ParallaxOcclusionApp::Shutdown()
{
	// Application cleanup on shutdown
}


void ParallaxOcclusionApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffer();
}


void ParallaxOcclusionApp::UpdateUI()
{
	if (m_uiOverlay->Header("Settings"))
	{
		m_uiOverlay->ComboBox("Mode", &m_psConstants.mappingMode, mappingModes);
	}
}


void ParallaxOcclusionApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(GetDepthBuffer(), ResourceState::DepthWrite);
	context.ClearColor(GetColorBuffer());
	context.ClearDepth(GetDepthBuffer());

	context.BeginRendering(GetColorBuffer(), GetDepthBuffer());

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());

	context.SetRootSignature(m_rootSignature);

	context.BindDescriptorHeap(GetDefaultDescriptorHeap());

	context.SetRootCBV(0, m_vsConstantBuffer);
	context.SetDescriptors(1, m_descriptorSet);

	context.SetGraphicsPipeline(m_graphicsPipeline);

	m_planeModel->Render(context);

	context.EndRendering();

	RenderUI(context);

	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void ParallaxOcclusionApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		256.0f);
	m_camera.SetPosition(Math::Vector3(0.0f, 3.0f, 3.0f));

	InitRootSignature();

	m_vsConstantBuffer = CreateConstantBuffer("VS Constant Buffer", 1, sizeof(VSConstants));
	m_psConstantBuffer = CreateConstantBuffer("PS Constant Buffer", 1, sizeof(PSConstants));

	LoadAssets();

	InitDescriptorSet();

	auto box = m_planeModel->boundingBox;

	m_controller.SetSpeedScale(0.01f);
	m_controller.RefreshFromCamera();
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.SetOrbitTarget(box.GetCenter(), 2.0f, 1.0f);
}


void ParallaxOcclusionApp::CreateWindowSizeDependentResources()
{
	// Create any resources that depend on window size.  May be called when the window size changes.
	if (!m_pipelineCreated)
	{
		InitPipeline();
		m_pipelineCreated = true;
	}

	m_camera.SetAspectRatio(GetWindowAspectRatio());
}


void ParallaxOcclusionApp::InitRootSignature()
{
	RootSignatureDesc desc{
		.name			= "Root Signature",
		.rootParameters = {
			RootCBV(0, ShaderStage::Vertex),
			Table({ TextureSRV(0, 2), ConstantBuffer }, ShaderStage::Pixel)
		},
		.staticSamplers = { StaticSampler(CommonStates::SamplerLinearWrap()) }
	};

	m_rootSignature = CreateRootSignature(desc);
}


void ParallaxOcclusionApp::InitPipeline()
{
	VertexStreamDesc vertexStreamDesc{
		.inputSlot = 0,
		.stride = sizeof(Vertex),
		.inputClassification = InputClassification::PerVertexData
	};

	vector<VertexElementDesc> vertexElements{
		{ "POSITION", 0, Format::RGB32_Float,   0, offsetof(Vertex, pos),       InputClassification::PerVertexData, 0 },
		{ "NORMAL",   0, Format::RGB32_Float,   0, offsetof(Vertex, normal),    InputClassification::PerVertexData, 0 },
		{ "TEXCOORD", 0, Format::RGB32_Float,   0, offsetof(Vertex, tangent),   InputClassification::PerVertexData, 0 },
		{ "TEXCOORD", 1, Format::RG32_Float,    0, offsetof(Vertex, uv),        InputClassification::PerVertexData, 0 }
	};

	// Mesh pipeline
	GraphicsPipelineDesc desc{
		.name				= "Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState	= CommonStates::RasterizerDefault(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "ParallaxVS" },
		.pixelShader		= { .shaderFile = "ParallaxPS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= vertexElements,
		.rootSignature		= m_rootSignature
	};

	m_graphicsPipeline = CreateGraphicsPipeline(desc);
}


void ParallaxOcclusionApp::InitDescriptorSet()
{
	m_descriptorSet = m_rootSignature->CreateDescriptorSet(1,
		{ m_colorTexture,
		  m_normalHeightTexture,
		  m_psConstantBuffer });
}


void ParallaxOcclusionApp::LoadAssets()
{
	using enum VertexComponent;
	auto layout = VertexLayout<Position | Texcoord0 | Normal | Tangent>();

	m_planeModel = LoadModel("plane.gltf", layout);

	m_colorTexture = LoadTexture("rocks_color_rgba.ktx");
	m_normalHeightTexture = LoadTexture("rocks_normal_height_rgba.ktx");
}


void ParallaxOcclusionApp::UpdateConstantBuffer()
{
	// VS constants
	m_vsConstants.projection = m_camera.GetProjectionMatrix();
	m_vsConstants.view = m_camera.GetViewMatrix();
	m_vsConstants.model = Matrix4::MakeScale(0.2f);
	m_vsConstants.cameraPos = Vector4(m_camera.GetPosition(), 1.0f);

	m_vsConstantBuffer->Update(sizeof(VSConstants), &m_vsConstants);

	// PS constants
	m_psConstantBuffer->Update(sizeof(PSConstants), &m_psConstants);
}