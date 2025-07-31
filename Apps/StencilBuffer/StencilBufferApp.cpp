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

#include "StencilBufferApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"

using namespace Luna;
using namespace Math;
using namespace std;


StencilBufferApp::StencilBufferApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{}


int StencilBufferApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void StencilBufferApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void StencilBufferApp::Startup()
{
	m_showGrid = true;
}


void StencilBufferApp::Shutdown()
{
	// Application cleanup on shutdown
}


void StencilBufferApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffer();
}


void StencilBufferApp::UpdateUI()
{
	if (m_uiOverlay->Header("Settings"))
	{
		m_uiOverlay->InputFloat("Outline width", &m_constants.outlineWidth, 0.05f);
	}
}


void StencilBufferApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(GetDepthBuffer(), ResourceState::DepthWrite);
	context.ClearColor(GetColorBuffer());
	context.ClearDepthAndStencil(GetDepthBuffer());

	context.BeginRendering(GetColorBuffer(), GetDepthBuffer());

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());

	context.SetRootSignature(m_rootSignature);

	context.SetDescriptors(0, m_cbvDescriptorSet);

	context.SetStencilRef(1);

	// Draw model toon-shaded to setup stencil mask
	{
		context.SetGraphicsPipeline(m_toonPipeline);
		m_model->Render(context);
	}

	// Draw stenciled outline
	{
		context.SetGraphicsPipeline(m_outlinePipeline);
		m_model->Render(context);
	}

	RenderGrid(context);
	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void StencilBufferApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		512.0f);
	Vector3 cameraPosition{ 4.75f, 3.25f, 4.75f };
	m_camera.SetPosition(cameraPosition);

	InitRootSignature();
	
	// Create and initialize constant buffer
	m_constantBuffer = CreateConstantBuffer("Constant Buffer", 1, sizeof(Constants));
	UpdateConstantBuffer();

	LoadAssets();

	InitDescriptorSet();

	BoundingBox box = m_model->boundingBox;

	m_controller.RefreshFromCamera();
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.SetOrbitTarget(box.GetCenter(), 4.0f, 0.25f);
	m_controller.SlowMovement(true);
	m_controller.SlowRotation(false);
	m_controller.SetSpeedScale(0.25f);
}


void StencilBufferApp::CreateWindowSizeDependentResources()
{
	// Create any resources that depend on window size.  May be called when the window size changes.
	if (!m_pipelinesCreated)
	{
		InitPipelines();
		m_pipelinesCreated = true;
	}

	// Update the camera since the aspect ratio might have changed
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		512.0f);
}


void StencilBufferApp::InitRootSignature()
{
	auto desc = RootSignatureDesc{
		.name				= "Root Signature",
		.flags				= RootSignatureFlags::AllowInputAssemblerInputLayout,
		.rootParameters		= {	RootCBV(0, ShaderStage::Vertex) }
	};

	m_rootSignature = CreateRootSignature(desc);
}


void StencilBufferApp::InitPipelines()
{
	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= sizeof(Vertex),
		.inputClassification	= InputClassification::PerVertexData
	};

	auto layout = VertexLayout<VertexComponent::PositionNormalColor>();

	// Toon pipeline
	{
		DepthStencilStateDesc depthStencilDesc = CommonStates::DepthStateReadWriteReversed();
		depthStencilDesc.stencilEnable = true;
		depthStencilDesc.backFace.stencilFunc = ComparisonFunc::Always;
		depthStencilDesc.backFace.stencilDepthFailOp = StencilOp::Replace;
		depthStencilDesc.backFace.stencilFailOp = StencilOp::Replace;
		depthStencilDesc.backFace.stencilPassOp = StencilOp::Replace;
		depthStencilDesc.frontFace = depthStencilDesc.backFace;

		GraphicsPipelineDesc toonPipelineDesc
		{
			.name				= "Toon Graphics PSO",
			.blendState			= CommonStates::BlendDisable(),
			.depthStencilState	= depthStencilDesc,
			.rasterizerState	= CommonStates::RasterizerDefault(),
			.rtvFormats			= { GetColorFormat() },
			.dsvFormat			= GetDepthFormat(),
			.topology			= PrimitiveTopology::TriangleList,
			.vertexShader		= { .shaderFile = "ToonVS" },
			.pixelShader		= { .shaderFile = "ToonPS" },
			.vertexStreams		= { vertexStreamDesc },
			.vertexElements		= layout.GetElements(),
			.rootSignature		= m_rootSignature
		};

		m_toonPipeline = CreateGraphicsPipeline(toonPipelineDesc);
	}

	// Outline pipeline
	{
		DepthStencilStateDesc depthStencilDesc = CommonStates::DepthStateReadWriteReversed();
		depthStencilDesc.depthEnable = false;
		depthStencilDesc.stencilEnable = true;
		depthStencilDesc.backFace.stencilFunc = ComparisonFunc::NotEqual;
		depthStencilDesc.backFace.stencilDepthFailOp = StencilOp::Keep;
		depthStencilDesc.backFace.stencilFailOp = StencilOp::Keep;
		depthStencilDesc.backFace.stencilPassOp = StencilOp::Replace;
		depthStencilDesc.frontFace = depthStencilDesc.backFace;

		GraphicsPipelineDesc outlinePipelineDesc
		{
			.name				= "Outline Graphics PSO",
			.blendState			= CommonStates::BlendDisable(),
			.depthStencilState	= depthStencilDesc,
			.rasterizerState	= CommonStates::RasterizerDefaultCW(),
			.rtvFormats			= { GetColorFormat() },
			.dsvFormat			= GetDepthFormat(),
			.topology			= PrimitiveTopology::TriangleList,
			.vertexShader		= {.shaderFile = "OutlineVS" },
			.pixelShader		= {.shaderFile = "OutlinePS" },
			.vertexStreams		= { vertexStreamDesc },
			.vertexElements		= layout.GetElements(),
			.rootSignature		= m_rootSignature
		};

		m_outlinePipeline = CreateGraphicsPipeline(outlinePipelineDesc);
	}
}


void StencilBufferApp::InitDescriptorSet()
{
	m_cbvDescriptorSet = m_rootSignature->CreateDescriptorSet(0);
	m_cbvDescriptorSet->SetCBV(0, m_constantBuffer);
}


void StencilBufferApp::UpdateConstantBuffer()
{
	m_constants.viewProjectionMatrix = m_camera.GetViewProjectionMatrix();
	m_constants.modelMatrix = Matrix4(kIdentity);
	m_constants.lightPos = Vector4(0.0f, 2.0f, 1.0f, 1.0f);

	m_constantBuffer->Update(sizeof(m_constants), &m_constants);
}


void StencilBufferApp::LoadAssets()
{
	auto layout = VertexLayout<VertexComponent::PositionNormalColor>();

	m_model = LoadModel("venus.gltf", layout, 1.8f);
}