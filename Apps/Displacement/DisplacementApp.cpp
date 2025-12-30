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

#include "DisplacementApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"

using namespace Luna;
using namespace Math;
using namespace std;


DisplacementApp::DisplacementApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{}


int DisplacementApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void DisplacementApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void DisplacementApp::Startup()
{
	// Application initialization, after device creation
}


void DisplacementApp::Shutdown()
{
	// Application cleanup on shutdown
}


void DisplacementApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffers();
}


void DisplacementApp::UpdateUI()
{
	if (m_uiOverlay->Header("Settings"))
	{
		m_uiOverlay->CheckBox("Tessellation displacement", &m_displacement);
		m_uiOverlay->InputFloat("Strength", &m_dsConstants.tessStrength, 0.025f);
		m_uiOverlay->InputFloat("Level", &m_hsConstants.tessLevel, 0.5f);
		m_uiOverlay->CheckBox("Splitscreen", &m_split);
	}
}


void DisplacementApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(GetDepthBuffer(), ResourceState::DepthWrite);
	context.ClearColor(GetColorBuffer());
	context.ClearDepth(GetDepthBuffer());

	context.BeginRendering(GetColorBuffer(), GetDepthBuffer());

	context.SetViewport(0.0f, 0.0f, (float)GetWindowWidth(), (float)GetWindowHeight());

	context.SetRootSignature(m_rootSignature);

	context.SetRootCBV(0, m_hsConstantBuffer);
	context.SetDescriptors(1, m_dsCbvSrvDescriptorSet);
	context.SetDescriptors(2, m_psSrvDescriptorSet);

	// Wireframe
	if (m_split)
	{
		context.SetGraphicsPipeline(m_wireframePipeline);
		context.SetScissor(0u, 0u, GetWindowWidth() / 2, GetWindowHeight());
		m_model->Render(context);
	}

	// Opaque
	{
		context.SetGraphicsPipeline(m_pipeline);
		context.SetScissor(m_split ? GetWindowWidth() / 2 : 0u, 0u, GetWindowWidth(), GetWindowHeight());
		m_model->Render(context);
	}

	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void DisplacementApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(45.0f),
		GetWindowAspectRatio(),
		0.1f,
		256.0f);
	m_camera.SetPosition(Vector3(-0.830579f, 0.427525f, 0.830579f));

	m_controller.SetSpeedScale(0.0025f);
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.SetOrbitTarget(Vector3(0.0f, 0.0f, 0.0f), Length(m_camera.GetPosition()), 0.25f);

	InitRootSignature();
	InitConstantBuffers();

	UpdateConstantBuffers();

	LoadAssets();

	InitDescriptorSets();
}


void DisplacementApp::CreateWindowSizeDependentResources()
{
	if (!m_pipelinesCreated)
	{
		InitPipelines();
		m_pipelinesCreated = true;
	}

	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(45.0f),
		GetWindowAspectRatio(),
		0.1f,
		256.0f);
}


void DisplacementApp::InitRootSignature()
{
	RootSignatureDesc rootSignatureDesc{
		.name				= "Geom Root Signature",
		.rootParameters		= {	
			RootCBV(0, ShaderStage::Hull),
			Table({ ConstantBuffer, TextureSRV }, ShaderStage::Domain),
			Table({ TextureSRV }, ShaderStage::Pixel)
		},
		.staticSamplers		= { StaticSampler(CommonStates::SamplerLinearWrap()) }
	};
	m_rootSignature = CreateRootSignature(rootSignatureDesc);
}


void DisplacementApp::InitPipelines()
{
	auto vertexLayout = VertexLayout<VertexComponent::PositionNormalTexcoord>();

	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= vertexLayout.GetSizeInBytes(),
		.inputClassification	= InputClassification::PerVertexData
	};

	GraphicsPipelineDesc pipelineDesc{
		.name				= "Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState	= CommonStates::RasterizerDefaultCW(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::PatchList_3_ControlPoint,
		.vertexShader		= { .shaderFile = "BaseVS" },
		.pixelShader		= { .shaderFile = "BasePS" },
		.hullShader			= { .shaderFile = "DisplacementHS"},
		.domainShader		= { .shaderFile = "DisplacementDS"},
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= vertexLayout.GetElements(),
		.rootSignature		= m_rootSignature
	};
	m_pipeline = CreateGraphicsPipeline(pipelineDesc);

	GraphicsPipelineDesc wireframePipelineDesc = pipelineDesc;
	wireframePipelineDesc.SetName("Wireframe PSO");
	wireframePipelineDesc.SetRasterizerState(CommonStates::RasterizerWireframe());
	m_wireframePipeline = CreateGraphicsPipeline(wireframePipelineDesc);
}


void DisplacementApp::InitConstantBuffers()
{
	// Hull shader constant buffer
	m_hsConstants.tessLevel = 64.0f;

	m_hsConstantBuffer = CreateConstantBuffer("HS Constant Buffer", 1, sizeof(HSConstants), &m_hsConstants);

	// Domain shader constant buffer
	m_dsConstants.lightPos = Math::Vector4(0.0f, 5.0f, 0.0f, 0.0f);
	m_dsConstants.tessAlpha = 1.0f;
	m_dsConstants.tessStrength = 0.1f;

	m_dsConstantBuffer = CreateConstantBuffer("DS Constant Buffer", 1, sizeof(DSConstants), &m_dsConstants);
}


void DisplacementApp::InitDescriptorSets()
{
	m_dsCbvSrvDescriptorSet = m_rootSignature->CreateDescriptorSet(1);
	m_dsCbvSrvDescriptorSet->SetCBV(0, m_dsConstantBuffer);
	m_dsCbvSrvDescriptorSet->SetSRV(0, m_texture);

	m_psSrvDescriptorSet = m_rootSignature->CreateDescriptorSet(2);
	m_psSrvDescriptorSet->SetSRV(0, m_texture);
}


void DisplacementApp::UpdateConstantBuffers()
{
	float savedLevel = m_hsConstants.tessLevel;
	if (!m_displacement)
	{
		m_hsConstants.tessLevel = 1.0f;
	}
	m_hsConstantBuffer->Update(sizeof(m_hsConstants), &m_hsConstants);

	if (!m_displacement)
	{
		m_hsConstants.tessLevel = savedLevel;
	}

	m_dsConstants.lightPos.SetY(5.0f - m_dsConstants.tessStrength);
	m_dsConstants.projectionMatrix = m_camera.GetProjectionMatrix();
	m_dsConstants.modelMatrix = m_camera.GetViewMatrix();
	m_dsConstantBuffer->Update(sizeof(m_dsConstants), &m_dsConstants);
}


void DisplacementApp::LoadAssets()
{
	m_texture = LoadTexture("stonefloor03_color_height_rgba.ktx");

	auto layout = VertexLayout<VertexComponent::PositionNormalTexcoord>();
	m_model = LoadModel("displacement_plane.gltf", layout, 1.0f);
}