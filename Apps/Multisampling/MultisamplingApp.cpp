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

#include "MultisamplingApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"

using namespace Luna;
using namespace Math;
using namespace std;


MultisamplingApp::MultisamplingApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{}


int MultisamplingApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void MultisamplingApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void MultisamplingApp::Startup()
{
	// Application initialization, after device creation
}


void MultisamplingApp::Shutdown()
{
	// Application cleanup on shutdown
}


void MultisamplingApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffer();
}


void MultisamplingApp::UpdateUI()
{
	if (m_uiOverlay->Header("Settings"))
	{
		m_uiOverlay->CheckBox("Sample rate shading", &m_sampleRateShading);
	}
}


void MultisamplingApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	Color clearColor{ DirectX::Colors::White };
	context.TransitionResource(m_msaaColorBuffer, ResourceState::RenderTarget);
	context.TransitionResource(m_msaaDepthBuffer, ResourceState::DepthWrite);
	context.ClearColor(m_msaaColorBuffer, clearColor);
	context.ClearDepth(m_msaaDepthBuffer);

	context.BeginRendering(m_msaaColorBuffer, m_msaaDepthBuffer);

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());

	context.SetRootSignature(m_rootSignature);
	context.SetGraphicsPipeline(m_sampleRateShading ? m_msaaSampleRatePipeline : m_msaaPipeline);

	// Render model
	{
		uint32_t meshIndex = 0;
		for (const auto& mesh : m_model->meshes)
		{
			context.SetIndexBuffer(mesh->indexBuffer);
			context.SetVertexBuffer(0, mesh->vertexBuffer);

			context.SetDescriptors(0, m_cbvDescriptorSet);
			context.SetDescriptors(1, m_srvDescriptorSets[meshIndex]);

			for (const auto& meshPart : mesh->meshParts)
			{
				context.DrawIndexed(meshPart.indexCount, meshPart.indexBase, meshPart.vertexBase);
			}
			++meshIndex;
		}
	}

	context.EndRendering();

	// Resolve the MSAA buffer to the back buffer, before rendering UI
	context.TransitionResource(m_msaaColorBuffer, ResourceState::ResolveSource);
	context.TransitionResource(GetColorBuffer(), ResourceState::ResolveDest);

	context.Resolve(m_msaaColorBuffer, GetColorBuffer(), GetColorFormat());

	// Render UI after MSAA resolve
	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(GetDepthBuffer(), ResourceState::DepthWrite);

	context.BeginRendering(GetColorBuffer(), GetDepthBuffer());
	RenderUI(context);
	context.EndRendering();

	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void MultisamplingApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		256.0f);
	m_camera.SetPosition(Vector3(4.838f, -3.23f, 7.05f));

	m_controller.SetSpeedScale(0.01f);
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.RefreshFromCamera();
	m_controller.SetOrbitTarget(Vector3(0.0f, 3.5f, 0.0f), Length(m_camera.GetPosition()), 0.25f);

	InitRootSignature();
	
	// Create constant buffer
	m_constantBuffer = CreateConstantBuffer("Constant Buffer", 1, sizeof(Constants));

	LoadAssets();

	InitDescriptorSets();
}


void MultisamplingApp::CreateWindowSizeDependentResources()
{
	InitRenderTargets();
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
		256.0f);
}


void MultisamplingApp::InitRenderTargets()
{
	ColorBufferDesc msaaColorBufferDesc{
		.name			= "MSAA Color Buffer",
		.resourceType	= ResourceType::Texture2DMS,
		.width			= GetWindowWidth(),
		.height			= GetWindowHeight(),
		.numSamples		= m_numSamples,
		.format			= GetColorFormat(),
		.clearColor		= DirectX::Colors::White
	};
	m_msaaColorBuffer = CreateColorBuffer(msaaColorBufferDesc);

	DepthBufferDesc msaaDepthBufferDesc{
		.name			= "MSAA Depth Buffer",
		.resourceType	= ResourceType::Texture2DMS,
		.width			= GetWindowWidth(),
		.height			= GetWindowHeight(),
		.numSamples		= m_numSamples,
		.format			= GetDepthFormat()
	};
	m_msaaDepthBuffer = CreateDepthBuffer(msaaDepthBufferDesc);
}


void MultisamplingApp::InitRootSignature()
{
	auto rootSignatureDesc = RootSignatureDesc{
		.name				= "Root Signature",
		.rootParameters		= {	
			RootCBV(0, ShaderStage::Vertex),
			Table({ TextureSRV }, ShaderStage::Pixel)
		},
		.staticSamplers		= { StaticSampler(CommonStates::SamplerLinearClamp()) }
	};

	m_rootSignature = CreateRootSignature(rootSignatureDesc);
}


void MultisamplingApp::InitPipelines()
{
	auto layout = VertexLayout<VertexComponent::PositionNormalColorTexcoord>();

	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= layout.GetSizeInBytes(),
		.inputClassification	= InputClassification::PerVertexData
	};

	RasterizerStateDesc rasterizerDesc = CommonStates::RasterizerDefault();
	rasterizerDesc.multisampleEnable = true;

	GraphicsPipelineDesc msaaPipelineDesc{
		.name				= "MSAA Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState	= rasterizerDesc,
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.msaaCount			= m_numSamples,
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "MeshVS" },
		.pixelShader		= { .shaderFile = "MeshPS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= layout.GetElements(),
		.rootSignature		= m_rootSignature
	};
	m_msaaPipeline = CreateGraphicsPipeline(msaaPipelineDesc);

	GraphicsPipelineDesc msaaSampleRatePipelineDesc = msaaPipelineDesc;
	msaaSampleRatePipelineDesc.SetName("MSAA Sample Rate Graphics PSO");
	msaaSampleRatePipelineDesc.sampleRateShading = true;
	m_msaaSampleRatePipeline = CreateGraphicsPipeline(msaaSampleRatePipelineDesc);
}


void MultisamplingApp::InitDescriptorSets()
{
	for (uint32_t i = 0; i < (uint32_t)m_model->meshes.size(); ++i)
	{
		const uint32_t materialIndex = m_model->meshes[i]->materialIndex;
		TexturePtr texture = m_model->materials[materialIndex].diffuseTexture;

		auto srvDescriptorSet = m_rootSignature->CreateDescriptorSet(1);
		srvDescriptorSet->SetSRV(0, texture);

		m_srvDescriptorSets.push_back(srvDescriptorSet);
	}

	m_cbvDescriptorSet = m_rootSignature->CreateDescriptorSet(0);
	m_cbvDescriptorSet->SetCBV(0, m_constantBuffer);
}


void MultisamplingApp::LoadAssets()
{
	auto layout = VertexLayout<VertexComponent::PositionNormalColorTexcoord>();

	m_model = LoadModel("voyager.gltf", layout, 1.0f, ModelLoad::StandardDefault, true);
}


void MultisamplingApp::UpdateConstantBuffer()
{
	m_constants.viewProjectionMatrix = m_camera.GetProjectionMatrix();
	m_constants.modelMatrix = m_camera.GetViewMatrix();
	m_constants.lightPos = Vector4(5.0f, 5.0f, 5.0f, 1.0f);

	m_constantBuffer->Update(sizeof(Constants), &m_constants);
}