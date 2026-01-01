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

#include "MeshShaderApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"
#include "Graphics\Device.h"
#include "Graphics\DeviceCaps.h"

using namespace Luna;
using namespace Math;
using namespace std;


MeshShaderApp::MeshShaderApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{}


int MeshShaderApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void MeshShaderApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void MeshShaderApp::Startup()
{
	if (!GetDevice()->GetDeviceCaps().features.meshShader)
	{
		Utility::ExitFatal("Mesh shader support is required for this sample.", "Missing feature");
	}
}


void MeshShaderApp::Shutdown()
{
	// Application cleanup on shutdown
}


void MeshShaderApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffer();
}


void MeshShaderApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(GetDepthBuffer(), ResourceState::DepthWrite);
	context.ClearColor(GetColorBuffer());
	context.ClearDepth(GetDepthBuffer());

	context.BeginRendering(GetColorBuffer(), GetDepthBuffer());

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());

	context.SetRootSignature(m_rootSignature);
	context.SetMeshletPipeline(m_meshletPipeline);

	context.SetDescriptors(0, m_cbvDescriptorSet);

	context.DispatchMesh(1, 1, 1);

	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void MeshShaderApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		256.0f);
	m_camera.SetPosition(Vector3(-2.0f, 0.0f, 4.0f));

	m_controller.SetSpeedScale(0.025f);
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.RefreshFromCamera();
	m_controller.SetOrbitTarget(Vector3(0.0f, 0.0f, 1.0f), Length(m_camera.GetPosition()), 0.25f);

	m_constantBuffer = CreateConstantBuffer("Constant Buffer", 1, sizeof(Constants));

	InitRootSignature();
	InitDescriptorSet();
}


void MeshShaderApp::CreateWindowSizeDependentResources()
{
	if (!m_pipelineCreated)
	{
		InitPipeline();
		m_pipelineCreated = true;
	}

	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		256.0f);
}


void MeshShaderApp::InitRootSignature()
{
	RootSignatureDesc desc{
		.name				= "Root Signature",
		.rootParameters		= {
			Table({ ConstantBuffer }, ShaderStage::Mesh)
		}
	};
	m_rootSignature = CreateRootSignature(desc);
}


void MeshShaderApp::InitPipeline()
{
	MeshletPipelineDesc desc{
		.name					= "Meshlet Pipeline",
		.blendState				= CommonStates::BlendDisable(),
		.depthStencilState		= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState		= CommonStates::RasterizerTwoSided(),
		.rtvFormats				= { GetColorFormat()},
		.dsvFormat				= GetDepthFormat(),
		.topology				= PrimitiveTopology::TriangleList,
		.amplificationShader	= { .shaderFile = "MeshShaderAS" },
		.meshShader				= { .shaderFile = "MeshShaderMS" },
		.pixelShader			= { .shaderFile = "MeshShaderPS" },
		.rootSignature			= m_rootSignature
	};
	m_meshletPipeline = CreateMeshletPipeline(desc);
}


void MeshShaderApp::InitDescriptorSet()
{
	m_cbvDescriptorSet = m_rootSignature->CreateDescriptorSet(0);
	m_cbvDescriptorSet->SetCBV(0, m_constantBuffer);
}


void MeshShaderApp::UpdateConstantBuffer()
{
	m_constants.projectionMatrix = m_camera.GetProjectionMatrix();
	m_constants.viewMatrix = m_camera.GetViewMatrix();
	m_constants.modelMatrix = Matrix4{ kIdentity };

	m_constantBuffer->Update(sizeof(Constants), &m_constants);
}