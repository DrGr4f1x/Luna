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

#include "GeometryShaderApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"

using namespace Luna;
using namespace Math;
using namespace std;


GeometryShaderApp::GeometryShaderApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{}


int GeometryShaderApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void GeometryShaderApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void GeometryShaderApp::Startup()
{}


void GeometryShaderApp::Shutdown()
{
	// Application cleanup on shutdown
}


void GeometryShaderApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffer();
}


void GeometryShaderApp::UpdateUI()
{
	if (m_uiOverlay->Header("Settings"))
	{
		m_uiOverlay->CheckBox("Display normals", &m_showNormals);

		if (m_uiOverlay->Header("Camera"))
		{
			char str[256];
			auto position = m_camera.GetPosition();
			sprintf_s(str, "(%2.2f, %2.2f, %2.2f)", (double)position.GetX(), (double)position.GetY(), (double)position.GetZ());
			m_uiOverlay->Text(str);
		}
	}
}


void GeometryShaderApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(GetDepthBuffer(), ResourceState::DepthWrite);
	context.ClearColor(GetColorBuffer());
	context.ClearDepth(GetDepthBuffer());

	context.BeginRendering(GetColorBuffer(), GetDepthBuffer());

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());

	context.SetRootSignature(m_meshRootSignature);
	context.SetGraphicsPipeline(m_meshPipeline);

	context.SetResources(m_meshResources);

	m_model->Render(context);

	if (m_showNormals)
	{
		context.SetRootSignature(m_geomRootSignature);
		context.SetGraphicsPipeline(m_geomPipeline);

		context.SetResources(m_geomResources);

		m_model->Render(context);
	}

	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void GeometryShaderApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.001f,
		256.0f);
	m_camera.SetPosition(Math::Vector3(0.0f, 0.9f, 3.9f));

	InitRootSignatures();
	
	// Create constant buffer
	m_constantBuffer = CreateConstantBuffer("Constant Buffer", 1, sizeof(Constants));

	LoadAssets();
	InitResourceSets();

	auto box = m_model->boundingBox;

	m_controller.SetSpeedScale(0.01f);
	m_controller.RefreshFromCamera();
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.SetOrbitTarget(box.GetCenter(), 4.0f, 2.0f);
}


void GeometryShaderApp::CreateWindowSizeDependentResources()
{
	if (!m_pipelinesCreated)
	{
		InitPipelines();
		m_pipelinesCreated = true;
	}

	// Update the camera since the aspect ratio might have changed
	m_camera.SetPerspectiveMatrix(
		XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.001f,
		256.0f);
}


void GeometryShaderApp::InitRootSignatures()
{
	RootSignatureDesc meshRootSignatureDesc{
		.name				= "Mesh Root Signature",
		.flags				= RootSignatureFlags::AllowInputAssemblerInputLayout,
		.rootParameters		= {	RootCBV(0, ShaderStage::Vertex) }
	};

	m_meshRootSignature = CreateRootSignature(meshRootSignatureDesc);

	RootSignatureDesc geomRootSignatureDesc{
		.name				= "Geom Root Signature",
		.flags				= RootSignatureFlags::AllowInputAssemblerInputLayout,
		.rootParameters		= {	RootCBV(0, ShaderStage::Geometry) }
	};

	m_geomRootSignature = CreateRootSignature(geomRootSignatureDesc);
}


void GeometryShaderApp::InitPipelines()
{
	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= sizeof(Vertex),
		.inputClassification	= InputClassification::PerVertexData
	};

	auto layout = VertexLayout<VertexComponent::PositionNormalColor>();

	// Mesh pipeline
	GraphicsPipelineDesc meshPipelineDesc{
		.name				= "Mesh Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState	= CommonStates::RasterizerDefault(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "MeshVS" },
		.pixelShader		= { .shaderFile = "MeshPS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= layout.GetElements(),
		.rootSignature		= m_meshRootSignature
	};

	m_meshPipeline = CreateGraphicsPipeline(meshPipelineDesc);

	// Geometry shader pipeline
	GraphicsPipelineDesc geomPipelineDesc{
		.name				= "Geom Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState	= CommonStates::RasterizerDefault(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "BaseVS" },
		.pixelShader		= { .shaderFile = "BasePS" },
		.geometryShader		= { .shaderFile = "BaseGS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= layout.GetElements(),
		.rootSignature		= m_geomRootSignature
	};

	m_geomPipeline = CreateGraphicsPipeline(geomPipelineDesc);
}


void GeometryShaderApp::InitResourceSets()
{
	m_meshResources.Initialize(m_meshRootSignature);
	m_meshResources.SetCBV(0, 0, m_constantBuffer);

	m_geomResources.Initialize(m_geomRootSignature);
	m_geomResources.SetCBV(0, 0, m_constantBuffer);
}


void GeometryShaderApp::UpdateConstantBuffer()
{
	m_constants.projectionMatrix = m_camera.GetProjectionMatrix();
	m_constants.modelMatrix = m_camera.GetViewMatrix();

	m_constantBuffer->Update(sizeof(m_constants), &m_constants);
}


void GeometryShaderApp::LoadAssets()
{
	auto layout = VertexLayout<VertexComponent::PositionNormalColor>();

	m_model = LoadModel("suzanne.gltf", layout, 2.5f);
}