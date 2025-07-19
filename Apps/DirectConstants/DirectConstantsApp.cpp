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

#include "DirectConstantsApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"

using namespace Luna;
using namespace Math;
using namespace std;


DirectConstantsApp::DirectConstantsApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
	, m_controller{ m_camera, Vector3(kYUnitVector) }
{}


int DirectConstantsApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void DirectConstantsApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void DirectConstantsApp::Startup()
{
	// Application initialization, after device creation
}


void DirectConstantsApp::Shutdown()
{
	// Application cleanup on shutdown
}


void DirectConstantsApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffer();

	constexpr float r = 7.5f;
	const float scaledTime = (float)m_timer.GetTotalSeconds() * 0.125f;
	const float sin_t = sinf(DirectX::XMConvertToRadians(scaledTime * 360.0f));
	const float cos_t = cosf(DirectX::XMConvertToRadians(scaledTime * 360.0f));
	constexpr float y = 4.0f;

	m_lightConstants.lightPositions[0] = Math::Vector4(r * 1.1f * sin_t, y, r * 1.1f * cos_t, 1.0f);
	m_lightConstants.lightPositions[1] = Math::Vector4(-r * sin_t, y, -r * cos_t, 1.0f);
	m_lightConstants.lightPositions[2] = Math::Vector4(r * 0.85f * sin_t, y, -sin_t * 2.5f, 1.5f);
	m_lightConstants.lightPositions[3] = Math::Vector4(0.0f, y, r * 1.25f * cos_t, 1.5f);
	m_lightConstants.lightPositions[4] = Math::Vector4(r * 2.25f * cos_t, y, 0.0f, 1.25f);
	m_lightConstants.lightPositions[5] = Math::Vector4(r * 2.5f * cos_t, y, r * 2.5f * sin_t, 1.25f);
}


void DirectConstantsApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(m_depthBuffer, ResourceState::DepthWrite);
	context.ClearColor(GetColorBuffer());
	context.ClearDepth(m_depthBuffer);

	context.BeginRendering(GetColorBuffer(), m_depthBuffer);

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());

	context.SetRootSignature(m_rootSignature);
	context.SetGraphicsPipeline(m_pipeline);

	context.SetResources(m_resources);
	context.SetConstantArray(1, 24, &m_lightConstants);

	m_model->Render(context);

	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void DirectConstantsApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.001f,
		256.0f);
	m_camera.SetPosition(Math::Vector3(-18.65f, 14.4f, 18.65f));
	m_camera.Update();

	m_controller.SetSpeedScale(0.01f);
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.SetOrbitTarget(Vector3(0.0f, 0.0f, 0.0f), Length(m_camera.GetPosition()), 4.0f);

	InitRootSignature();
	InitConstantBuffer();

	LoadAssets();

	InitResourceSet();
}


void DirectConstantsApp::CreateWindowSizeDependentResources()
{
	InitDepthBuffer();
	if (!m_pipelineCreated)
	{
		InitPipeline();
		m_pipelineCreated = true;
	}
}


void DirectConstantsApp::InitDepthBuffer()
{
	DepthBufferDesc depthBufferDesc{
		.name		= "Depth Buffer",
		.width		= GetWindowWidth(),
		.height		= GetWindowHeight(),
		.format		= GetDepthFormat()
	};

	m_depthBuffer = CreateDepthBuffer(depthBufferDesc);
}


void DirectConstantsApp::InitRootSignature()
{
	RootSignatureDesc rootSignatureDesc{
		.name				= "Root Signature",
		.flags				= RootSignatureFlags::AllowInputAssemblerInputLayout,
		.rootParameters		= {	
			RootCBV(0, ShaderStage::Vertex),
			RootConstants(1, 24, ShaderStage::Vertex)
		}
	};

	m_rootSignature = CreateRootSignature(rootSignatureDesc);
}


void DirectConstantsApp::InitPipeline()
{
	auto vertexLayout = VertexLayout<VertexComponent::PositionNormalColor>();

	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= vertexLayout.GetSizeInBytes(),
		.inputClassification	= InputClassification::PerVertexData
	};

	GraphicsPipelineDesc pipelineDesc{
		.name				= "Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState	= CommonStates::RasterizerDefault(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "LightsVS" },
		.pixelShader		= { .shaderFile = "LightsPS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= vertexLayout.GetElements(),
		.rootSignature		= m_rootSignature
	};

	m_pipeline = CreateGraphicsPipeline(pipelineDesc);
}


void DirectConstantsApp::InitConstantBuffer()
{
	// Constant buffer
	GpuBufferDesc desc{
		.name			= "Constant Buffer",
		.resourceType	= ResourceType::ConstantBuffer,
		.memoryAccess	= MemoryAccess::GpuRead | MemoryAccess::CpuWrite,
		.elementCount	= 1,
		.elementSize	= sizeof(Constants)
	};
	m_constantBuffer = CreateGpuBuffer(desc);
}


void DirectConstantsApp::InitResourceSet()
{
	m_resources.Initialize(m_rootSignature);
	m_resources.SetCBV(0, 0, m_constantBuffer);
}


void DirectConstantsApp::UpdateConstantBuffer()
{
	m_constants.projectionMatrix = m_camera.GetProjectionMatrix();
	m_constants.modelMatrix = m_camera.GetViewMatrix();

	m_constantBuffer->Update(sizeof(m_constants), &m_constants);
}


void DirectConstantsApp::LoadAssets()
{
	auto layout = VertexLayout<VertexComponent::PositionNormalColor>();
	m_model = LoadModel("samplescene.gltf", layout, 1.0f);
}