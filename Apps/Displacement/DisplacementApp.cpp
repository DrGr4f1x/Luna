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
	, m_controller{ m_camera, Vector3(kYUnitVector) }
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

	auto colorBuffer = GetColorBuffer();
	context.TransitionResource(colorBuffer, ResourceState::RenderTarget);
	context.TransitionResource(m_depthBuffer, ResourceState::DepthWrite);
	context.ClearColor(colorBuffer);
	context.ClearDepth(m_depthBuffer);

	context.BeginRendering(colorBuffer, m_depthBuffer);

	context.SetViewport(0.0f, 0.0f, (float)GetWindowWidth(), (float)GetWindowHeight());

	context.SetRootSignature(m_rootSignature);

	context.SetResources(m_resources);

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
	context.TransitionResource(colorBuffer, ResourceState::Present);

	context.Finish();
}


void DisplacementApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(45.0f),
		GetWindowAspectRatio(),
		0.1f,
		256.0f);
	m_camera.SetPosition(Vector3(0.830579f, -0.427525f, -0.830579f));

	m_controller.SetSpeedScale(0.0025f);
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.SetOrbitTarget(Vector3(0.0f, 0.0f, 0.0f), Length(m_camera.GetPosition()), 0.25f);

	InitRootSignature();
	InitConstantBuffers();

	UpdateConstantBuffers();

	LoadAssets();

	InitResourceSet();
}


void DisplacementApp::CreateWindowSizeDependentResources()
{
	InitDepthBuffer();
	if (!m_pipelinesCreated)
	{
		InitPipelines();
		m_pipelinesCreated = true;
	}
}


void DisplacementApp::InitDepthBuffer()
{
	DepthBufferDesc depthBufferDesc{
		.name		= "Depth Buffer",
		.width		= GetWindowWidth(),
		.height		= GetWindowHeight(),
		.format		= GetDepthFormat()
	};

	m_depthBuffer = CreateDepthBuffer(depthBufferDesc);
}


void DisplacementApp::InitRootSignature()
{
	RootSignatureDesc rootSignatureDesc{
		.name = "Geom Root Signature",
		.flags = RootSignatureFlags::AllowInputAssemblerInputLayout,
		.rootParameters = {	
			RootParameter::RootCBV(0, ShaderStage::Hull),
			RootParameter::Table({ DescriptorRange::ConstantBuffer(0), DescriptorRange::TextureSRV(1) }, ShaderStage::Domain),
			RootParameter::Range(DescriptorType::TextureSRV, 0, 1, ShaderStage::Pixel),
			RootParameter::Range(DescriptorType::Sampler, 0, 1, ShaderStage::Domain),
			RootParameter::Range(DescriptorType::Sampler, 0, 1, ShaderStage::Pixel)
		}
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

	GpuBufferDesc hsConstantBufferDesc{
		.name			= "HS Constant Buffer",
		.resourceType	= ResourceType::ConstantBuffer,
		.memoryAccess	= MemoryAccess::GpuRead | MemoryAccess::CpuWrite,
		.elementCount	= 1,
		.elementSize	= sizeof(HSConstants),
		.initialData	= &m_hsConstants
	};
	m_hsConstantBuffer = CreateGpuBuffer(hsConstantBufferDesc);

	// Domain shader constant buffer
	m_dsConstants.lightPos = Math::Vector4(0.0f, 5.0f, 0.0f, 0.0f);
	m_dsConstants.tessAlpha = 1.0f;
	m_dsConstants.tessStrength = 0.1f;

	GpuBufferDesc dsConstantBufferDesc{
		.name			= "DS Constant Buffer",
		.resourceType	= ResourceType::ConstantBuffer,
		.memoryAccess	= MemoryAccess::GpuRead | MemoryAccess::CpuWrite,
		.elementCount	= 1,
		.elementSize	= sizeof(DSConstants),
		.initialData	= &m_dsConstants
	};
	m_dsConstantBuffer = CreateGpuBuffer(dsConstantBufferDesc);
}


void DisplacementApp::InitResourceSet()
{
	m_resources.Initialize(m_rootSignature);
	m_resources.SetCBV(0, 0, m_hsConstantBuffer);
	m_resources.SetCBV(1, 0, m_dsConstantBuffer);
	m_resources.SetSRV(1, 1, m_texture);
	m_resources.SetSRV(2, 0, m_texture);
	m_resources.SetSampler(3, 0, m_sampler);
	m_resources.SetSampler(4, 0, m_sampler);
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
	m_dsConstants.projectionMatrix = m_camera.GetProjMatrix();
	m_dsConstants.modelMatrix = m_camera.GetViewMatrix();
	m_dsConstantBuffer->Update(sizeof(m_dsConstants), &m_dsConstants);
}


void DisplacementApp::LoadAssets()
{
	m_texture = LoadTexture("stonefloor03_color_height_rgba.ktx");
	m_sampler = CreateSampler(CommonStates::SamplerLinearWrap());

	auto layout = VertexLayout<VertexComponent::PositionNormalTexcoord>();
	m_model = LoadModel("displacement_plane.gltf", layout, 1.0f);
}