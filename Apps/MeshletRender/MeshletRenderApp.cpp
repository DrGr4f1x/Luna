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

#include "MeshletRenderApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"
#include "Graphics\Device.h"
#include "Graphics\DeviceCaps.h"
#include "Graphics\DeviceManager.h"

using namespace Luna;
using namespace Math;
using namespace std;


MeshletRenderApp::MeshletRenderApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{
}


int MeshletRenderApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void MeshletRenderApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void MeshletRenderApp::Startup()
{
	if (!GetDevice()->GetDeviceCaps().features.meshShader)
	{
		Utility::ExitFatal("Mesh shader support is required for this sample.", "Missing feature");
	}
}


void MeshletRenderApp::Shutdown()
{
	// Application cleanup on shutdown
}


void MeshletRenderApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffer();
}


void MeshletRenderApp::Render()
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

	uint32_t meshIndex = 0;
	for (const auto& mesh : m_model)
	{
		context.SetConstant(1, 0, mesh.indexSize);
		context.SetDescriptors(2, m_srvDescriptorSets[meshIndex]);

		for (const auto& subset : mesh.meshletSubsets)
		{
			context.SetConstant(1, 1, subset.offset);
			context.DispatchMesh(subset.count, 1, 1);
		}

		++meshIndex;
	}

	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void MeshletRenderApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		1000.0f);
	m_camera.SetPosition(Vector3(0.0f, 75.0f, 150.0f));

	m_constantBuffer = CreateConstantBuffer("Constant Buffer", 1, sizeof(Constants));
	
	InitRootSignature();

	m_model.LoadFromFile("Dragon_LOD0.bin");
	m_model.InitResources(m_deviceManager->GetDevice());

	InitDescriptorSets();

	Vector3 center = m_model.GetBoundingSphere().Center;

	m_controller.SetSpeedScale(0.025f);
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.RefreshFromCamera();
	m_controller.SetOrbitTarget(center, Length(m_camera.GetPosition()), 0.25f);
}


void MeshletRenderApp::CreateWindowSizeDependentResources()
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
		1000.0f);
}


void MeshletRenderApp::InitRootSignature()
{
	RootSignatureDesc desc{
		.name = "Root Signature",
		.rootParameters = {
			Table({ ConstantBuffer }, ShaderStage::Mesh | ShaderStage::Pixel),
			RootConstants(1, 2, ShaderStage::Mesh),
			Table({ StructuredBufferSRV, StructuredBufferSRV, RawBufferSRV, StructuredBufferSRV }, ShaderStage::Mesh)
		}
	};
	m_rootSignature = CreateRootSignature(desc);
}


void MeshletRenderApp::InitPipeline()
{
	MeshletPipelineDesc desc{
		.name				= "Meshlet Pipeline",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState	= CommonStates::RasterizerDefaultCW(),
		.rtvFormats			= { GetColorFormat()},
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.meshShader			= { .shaderFile = "MeshletMS" },
		.pixelShader		= { .shaderFile = "MeshletPS" },
		.rootSignature		= m_rootSignature
	};
	m_meshletPipeline = CreateMeshletPipeline(desc);
}


void MeshletRenderApp::InitDescriptorSets()
{
	m_cbvDescriptorSet = m_rootSignature->CreateDescriptorSet(0);
	m_cbvDescriptorSet->SetCBV(0, m_constantBuffer);

	m_srvDescriptorSets.resize(m_model.GetMeshCount());
	for (uint32_t i = 0; i < (uint32_t)m_model.GetMeshCount(); ++i)
	{
		const auto& mesh = m_model.GetMesh(i);

		m_srvDescriptorSets[i] = m_rootSignature->CreateDescriptorSet(2);
		m_srvDescriptorSets[i]->SetSRV(0, mesh.vertexResources[0]);
		m_srvDescriptorSets[i]->SetSRV(1, mesh.meshletResource);
		m_srvDescriptorSets[i]->SetSRV(2, mesh.uniqueVertexIndexResource);
		m_srvDescriptorSets[i]->SetSRV(3, mesh.primitiveIndexResource);
	}
}


void MeshletRenderApp::UpdateConstantBuffer()
{
	m_constants.modelMatrix = Matrix4{ kIdentity };
	m_constants.modelViewMatrix = m_camera.GetViewMatrix() * m_constants.modelMatrix;
	m_constants.modelViewProjectionMatrix = m_camera.GetProjectionMatrix() * m_constants.modelViewMatrix;
	m_constants.drawMeshlets = 1;

	m_constantBuffer->Update(sizeof(Constants), &m_constants);
}