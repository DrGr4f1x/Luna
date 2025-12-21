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

#include "TextureApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"

using namespace Luna;
using namespace std;


TextureApp::TextureApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{}


int TextureApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void TextureApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void TextureApp::Startup()
{
	// Setup camera
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		256.0f);
	m_camera.SetPosition(Math::Vector3(0.0f, 0.0f, -m_zoom));

	m_controller.SetSpeedScale(0.025f);
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.RefreshFromCamera();
	m_controller.SetOrbitTarget(Math::Vector3(0.0f, 0.0f, 0.0f), Length(m_camera.GetPosition()), 0.25f);
}


void TextureApp::Shutdown()
{
	// Application cleanup on shutdown
}


void TextureApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffer();
}


void TextureApp::UpdateUI()
{
	if (m_uiOverlay->Header("Settings"))
	{
		m_uiOverlay->SliderFloat("LOD bias", &m_constants.lodBias, 0.0f, (float)m_texture->GetNumMips());
	}
}


void TextureApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(GetDepthBuffer(), ResourceState::DepthWrite);
	context.ClearColor(GetColorBuffer());
	context.ClearDepth(GetDepthBuffer());

	context.BeginRendering(GetColorBuffer(), GetDepthBuffer());

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());

	context.SetRootSignature(m_rootSignature);
	context.SetGraphicsPipeline(m_graphicsPipeline);

	context.SetRootCBV(0, m_constantBuffer);
	context.SetDescriptors(1, m_srvDescriptorSet);

	context.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
	context.SetVertexBuffer(0, m_vertexBuffer);
	context.SetIndexBuffer(m_indexBuffer);

	context.DrawIndexed((uint32_t)m_indexBuffer->GetElementCount());

	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void TextureApp::CreateDeviceDependentResources()
{
	// Setup vertices for a single uv-mapped quad made from two triangles
	vector<Vertex> vertexData =
	{
		{ {  1.0f,  1.0f,  0.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
		{ { -1.0f,  1.0f,  0.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f } },
		{ { -1.0f, -1.0f,  0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
		{ {  1.0f, -1.0f,  0.0f }, { 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
	};
	m_vertexBuffer = CreateVertexBuffer<Vertex>("Vertex Buffer", vertexData);

	vector<uint32_t> indexData = { 0,1,2, 2,3,0 };
	m_indexBuffer = CreateIndexBuffer("Index Buffer", indexData);

	// Setup constant buffer
	m_constantBuffer = CreateConstantBuffer("Constant Buffer", 1, sizeof(m_constants));
	m_constants.modelMatrix = Math::Matrix4(Math::kIdentity);

	UpdateConstantBuffer();
	InitRootSignature();
	LoadAssets();
	InitDescriptorSet();
}


void TextureApp::CreateWindowSizeDependentResources()
{
	// Create any resources that depend on window size.  May be called when the window size changes.
	if (!m_pipelineCreated)
	{
		InitPipelineState();
		m_pipelineCreated = true;
	}

	// Update the camera since the aspect ratio might have changed
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		256.0f);
}


void TextureApp::InitRootSignature()
{
	auto rootSignatureDesc = RootSignatureDesc{
		.name				= "Root Signature",
		.rootParameters		= { 
			RootCBV(0, ShaderStage::Vertex),
			Table({ TextureSRV }, ShaderStage::Pixel)
		},
		.staticSamplers		= { StaticSampler(CommonStates::SamplerLinearClamp(), ShaderStage::Pixel) }
	};

	m_rootSignature = CreateRootSignature(rootSignatureDesc);
}


void TextureApp::InitPipelineState()
{
	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= sizeof(Vertex),
		.inputClassification	= InputClassification::PerVertexData
	};

	vector<VertexElementDesc> vertexElements{
		{ "POSITION", 0, Format::RGB32_Float, 0, offsetof(Vertex, position), InputClassification::PerVertexData, 0 },
		{ "TEXCOORD", 0, Format::RG32_Float, 0, offsetof(Vertex, uv), InputClassification::PerVertexData, 0 },
		{ "NORMAL", 0, Format::RGB32_Float, 0, offsetof(Vertex, normal), InputClassification::PerVertexData, 0 }
	};

	GraphicsPipelineDesc desc
	{
		.name				= "Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadOnlyReversed(),
		.rasterizerState	= CommonStates::RasterizerTwoSided(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "TextureVS" },
		.pixelShader		= { .shaderFile = "TexturePS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= vertexElements,
		.rootSignature		= m_rootSignature
	};

	m_graphicsPipeline = CreateGraphicsPipeline(desc);
}


void TextureApp::InitDescriptorSet()
{
	m_srvDescriptorSet = m_rootSignature->CreateDescriptorSet(1);
	m_srvDescriptorSet->SetSRV(0, m_texture);
}


void TextureApp::LoadAssets()
{
	if (m_appInfo.api == GraphicsApi::D3D12)
	{
		m_texture = LoadTexture("DirectXLogo.dds");
	}
	else
	{
		m_texture = LoadTexture("vulkan_cloth_rgba.ktx2");
		m_flipUVs = true;
	}
}


void TextureApp::UpdateConstantBuffer()
{
	using namespace Math;

	m_constants.viewProjectionMatrix = m_camera.GetViewProjectionMatrix();
	m_constants.modelMatrix = Matrix4(kIdentity);
	m_constants.viewPos = m_camera.GetPosition();
	m_constants.flipUVs = m_flipUVs;

	m_constantBuffer->Update(sizeof(Constants), &m_constants);
}