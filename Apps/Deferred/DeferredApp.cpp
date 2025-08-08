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

#include "DeferredApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"

using namespace Luna;
using namespace Math;
using namespace std;


DeferredApp::DeferredApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{}


int DeferredApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void DeferredApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void DeferredApp::Startup()
{
	// Application initialization, after device creation
}


void DeferredApp::Shutdown()
{
	// Application cleanup on shutdown
}


void DeferredApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffers();
}


void DeferredApp::UpdateUI()
{
	if (m_uiOverlay->Header("Settings")) 
	{
		m_uiOverlay->ComboBox("Display", &m_displayBuffer, { "Final composition", "Position", "Normals", "Albedo", "Specular" });
	}
}


void DeferredApp::Render()
{
	auto& context = GraphicsContext::Begin("Frame");

	// G-Buffer pass
	{
		ScopedDrawEvent offscreenEvent(context, "G-Buffer Pass");

		context.TransitionResource(m_positionBuffer, ResourceState::RenderTarget);
		context.TransitionResource(m_normalBuffer, ResourceState::RenderTarget);
		context.TransitionResource(m_albedoBuffer, ResourceState::RenderTarget);
		context.TransitionResource(m_depthBuffer, ResourceState::DepthWrite);
		context.ClearColor(m_positionBuffer);
		context.ClearColor(m_normalBuffer);
		context.ClearColor(m_albedoBuffer);
		context.ClearDepth(m_depthBuffer);

		context.BeginRendering({ m_positionBuffer, m_normalBuffer, m_albedoBuffer }, m_depthBuffer);

		context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());

		context.SetRootSignature(m_gbufferRootSignature);
		context.SetGraphicsPipeline(m_gbufferPipeline);

		// Render floor
		context.SetDescriptors(0, m_gbufferCbvDescriptorSet);
		context.SetDescriptors(1, m_floorSrvDescriptorSet);
		m_floorModel->Render(context);

		// Render armor model instanced
		context.SetDescriptors(1, m_armorSrvDescriptorSet);
		m_armorModel->RenderInstanced(context, 3);

		context.EndRendering();
	}

	context.TransitionResource(m_positionBuffer, ResourceState::PixelShaderResource);
	context.TransitionResource(m_normalBuffer, ResourceState::PixelShaderResource);
	context.TransitionResource(m_albedoBuffer, ResourceState::PixelShaderResource);
	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(GetDepthBuffer(), ResourceState::DepthWrite);

	context.BeginRendering(GetColorBuffer(), GetDepthBuffer());

	// Lighting pass
	{
		ScopedDrawEvent offscreenEvent(context, "Lighting Pass");

		context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());

		context.SetRootSignature(m_lightingRootSignature);
		context.SetGraphicsPipeline(m_lightingPipeline);

		context.SetDescriptors(0, m_lightingCbvSrvDescriptorSet);

		context.Draw(3);
	}

	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void DeferredApp::CreateDeviceDependentResources()
{
	// Create any resources that depend on the device, but not the window size
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		512.0f);
	m_camera.SetPosition({ -2.15f, 0.3f, 8.75f });

	m_controller.RefreshFromCamera();
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.SetOrbitTarget(Vector3(0.0f, 0.0f, 0.0f), 10.0f, 0.25f);
	m_controller.SlowMovement(true);
	m_controller.SlowRotation(false);
	m_controller.SetSpeedScale(0.25f);

	InitRootSignatures();
	InitConstantBuffers();
	LoadAssets();
}


void DeferredApp::CreateWindowSizeDependentResources()
{
	// Create any resources that depend on window size.  May be called when the window size changes.
	InitGBuffer();
	if (!m_pipelinesCreated)
	{
		InitPipelines();
		m_pipelinesCreated = true;
	}

	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		512.0f);

	InitDescriptorSets();
}


void DeferredApp::InitGBuffer()
{
	const uint32_t width = GetWindowWidth();
	const uint32_t height = GetWindowHeight();

	// Position buffer
	ColorBufferDesc positionBufferDesc{
		.name		= "G-Buffer Position",
		.width		= width,
		.height		= height,
		.format		= Format::RGBA16_Float
	};
	m_positionBuffer = CreateColorBuffer(positionBufferDesc);

	// Normal buffer
	ColorBufferDesc normalBufferDesc{
		.name		= "G-Buffer Normals",
		.width		= width,
		.height		= height,
		.format		= Format::RGBA16_Float
	};
	m_normalBuffer = CreateColorBuffer(normalBufferDesc);

	// Albedo buffer
	ColorBufferDesc albedoBufferDesc{
		.name		= "G-Buffer Albedo",
		.width		= width,
		.height		= height,
		.format		= Format::RGBA8_UNorm
	};
	m_albedoBuffer = CreateColorBuffer(albedoBufferDesc);

	// Depth buffer
	DepthBufferDesc depthBufferDesc{
		.name		= "G-Buffer Depth",
		.width		= width,
		.height		= height,
		.format		= GetDepthFormat()
	};
	m_depthBuffer = CreateDepthBuffer(depthBufferDesc);
}


void DeferredApp::InitRootSignatures()
{
	// G-Buffer pass root signature
	RootSignatureDesc gbufferDesc{
		.name				= "G-Buffer Root Signature",
		.rootParameters		= {
			RootCBV(0, ShaderStage::Vertex),
			Table({TextureSRV, TextureSRV}, ShaderStage::Pixel)
		},
		.staticSamplers		= {	StaticSampler(CommonStates::SamplerLinearWrap()) }
	};

	m_gbufferRootSignature = CreateRootSignature(gbufferDesc);

	// Lighting pass root signature
	RootSignatureDesc lightingDesc{
		.name				= "Lighting Root Signature",
		.rootParameters		= {
			Table({TextureSRV, TextureSRV, TextureSRV, ConstantBuffer}, ShaderStage::Pixel)
		},
		.staticSamplers		= { StaticSampler(CommonStates::SamplerLinearClamp()) }
	};

	m_lightingRootSignature = CreateRootSignature(lightingDesc);
}


void DeferredApp::InitPipelines()
{
	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= sizeof(Vertex),
		.inputClassification	= InputClassification::PerVertexData
	};

	vector<VertexElementDesc> vertexElements{
		{ "POSITION", 0, Format::RGB32_Float, 0, offsetof(Vertex, position), InputClassification::PerVertexData, 0 },
		{ "NORMAL", 0, Format::RGB32_Float, 0, offsetof(Vertex, normal), InputClassification::PerVertexData, 0 },
		{ "TANGENT", 0, Format::RGB32_Float, 0, offsetof(Vertex, tangent), InputClassification::PerVertexData, 0 },
		{ "COLOR", 0, Format::RGBA32_Float, 0, offsetof(Vertex, color), InputClassification::PerVertexData, 0 },
		{ "TEXCOORD", 0, Format::RG32_Float, 0, offsetof(Vertex, uv), InputClassification::PerVertexData, 0 }
	};

	GraphicsPipelineDesc gbufferGraphicsPipelineDesc{
		.name				= "G-Buffer Graphics Pipeline",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState	= CommonStates::RasterizerDefault(),
		.rtvFormats			= { Format::RGBA16_Float, Format::RGBA16_Float, Format::RGBA8_UNorm },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "GBufferVS" },
		.pixelShader		= { .shaderFile = "GBufferPS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= vertexElements,
		.rootSignature		= m_gbufferRootSignature
	};
	m_gbufferPipeline = CreateGraphicsPipeline(gbufferGraphicsPipelineDesc);

	GraphicsPipelineDesc lightingGraphicsPipelineDesc{
		.name				= "Lighting Graphics Pipeline",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateDisabled(),
		.rasterizerState	= CommonStates::RasterizerTwoSided(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "LightingVS" },
		.pixelShader		= { .shaderFile = "LightingPS" },
		.rootSignature		= m_lightingRootSignature
	};
	m_lightingPipeline = CreateGraphicsPipeline(lightingGraphicsPipelineDesc);
}


void DeferredApp::InitConstantBuffers()
{
	m_gbufferConstantBuffer = CreateConstantBuffer("G-Buffer Constant Buffer", 1, sizeof(GBufferConstants));
	m_lightingConstantBuffer = CreateConstantBuffer("Lighting Constant Buffer", 1, sizeof(LightingConstants));
}


void DeferredApp::InitDescriptorSets()
{
	m_gbufferCbvDescriptorSet = m_gbufferRootSignature->CreateDescriptorSet(0);
	m_gbufferCbvDescriptorSet->SetCBV(0, m_gbufferConstantBuffer);

	m_armorSrvDescriptorSet = m_gbufferRootSignature->CreateDescriptorSet(1);
	m_armorSrvDescriptorSet->SetSRV(0, m_armorColorTexture);
	m_armorSrvDescriptorSet->SetSRV(1, m_armorNormalTexture);

	m_floorSrvDescriptorSet = m_gbufferRootSignature->CreateDescriptorSet(1);
	m_floorSrvDescriptorSet->SetSRV(0, m_floorColorTexture);
	m_floorSrvDescriptorSet->SetSRV(1, m_floorNormalTexture);

	m_lightingCbvSrvDescriptorSet = m_lightingRootSignature->CreateDescriptorSet(0);
	m_lightingCbvSrvDescriptorSet->SetSRV(0, m_positionBuffer);
	m_lightingCbvSrvDescriptorSet->SetSRV(1, m_normalBuffer);
	m_lightingCbvSrvDescriptorSet->SetSRV(2, m_albedoBuffer);
	m_lightingCbvSrvDescriptorSet->SetCBV(3, m_lightingConstantBuffer);
}


void DeferredApp::LoadAssets()
{
	using enum VertexComponent;
	auto layout = VertexLayout<Position | Normal | Tangent | Color0 | Texcoord0>();

	m_armorModel = LoadModel("armor/armor.gltf", layout);
	m_floorModel = LoadModel("deferred_floor.gltf", layout);

	m_armorColorTexture = LoadTexture("armor/colormap_rgba.ktx");
	m_armorNormalTexture = LoadTexture("armor/normalmap_rgba.ktx");
	m_floorColorTexture = LoadTexture("stonefloor01_color_rgba.ktx");
	m_floorNormalTexture = LoadTexture("stonefloor01_normal_rgba.ktx");
}


void DeferredApp::UpdateConstantBuffers()
{
	// GBuffer constants
	m_gbufferConstants.projectionMatrix = m_camera.GetProjectionMatrix();
	m_gbufferConstants.viewMatrix = m_camera.GetViewMatrix();
	m_gbufferConstants.modelMatrix = Matrix4{ kIdentity };
	m_gbufferConstants.instancePositions[0] = Vector3(0.0f, 0.0f, 0.0f);
	m_gbufferConstants.instancePositions[1] = Vector3(-4.0f, 0.0f, -4.0f);
	m_gbufferConstants.instancePositions[2] = Vector3(4.0f, 0.0f, -4.0f);

	m_gbufferConstantBuffer->Update(sizeof(GBufferConstants), &m_gbufferConstants);

	// Lighting constants
	// Radius is packed into w component of colorAndRadius
	
	// White
	m_lightingConstants.lights[0].position = Vector4(0.0f, 0.0f, 1.0f, 0.0f);
	m_lightingConstants.lights[0].colorAndRadius = Vector4(1.0f, 1.0f, 1.0f, 15.0f * 0.25f);
	// Red
	m_lightingConstants.lights[1].position = Vector4(-2.0f, 0.0f, 0.0f, 0.0f);
	m_lightingConstants.lights[1].colorAndRadius = Vector4(1.0f, 0.0f, 0.0f, 15.0f);
	// Blue
	m_lightingConstants.lights[2].position = Vector4(2.0f, 1.0f, 0.0f, 0.0f);
	m_lightingConstants.lights[2].colorAndRadius = Vector4(0.0f, 0.0f, 2.5f, 5.0f);
	// Yellow
	m_lightingConstants.lights[3].position = Vector4(0.0f, 0.9f, 0.5f, 0.0f);
	m_lightingConstants.lights[3].colorAndRadius = Vector4(1.0f, 1.0f, 0.0f, 2.0f);
	// Green
	m_lightingConstants.lights[4].position = Vector4(0.0f, 0.5f, 0.0f, 0.0f);
	m_lightingConstants.lights[4].colorAndRadius = Vector4(0.0f, 1.0f, 0.2f, 5.0f);
	// Yellow
	m_lightingConstants.lights[5].position = Vector4(0.0f, 1.0f, 0.0f, 0.0f);
	m_lightingConstants.lights[5].colorAndRadius = Vector4(1.0f, 0.7f, 0.3f, 25.0f);

	// Animate the lights
	if (m_isRunning)
	{
		using namespace DirectX;
		const float time = 0.1f * (float)m_timer.GetTotalSeconds();

		m_lightingConstants.lights[0].position.SetX(sinf(XMConvertToRadians(360.0f * time)) * 5.0f);
		m_lightingConstants.lights[0].position.SetZ(cosf(XMConvertToRadians(360.0f * time)) * 5.0f);

		m_lightingConstants.lights[1].position.SetX(-4.0f + sinf(XMConvertToRadians(360.0f * time) + 45.0f) * 2.0f);
		m_lightingConstants.lights[1].position.SetZ(0.0f + cosf(XMConvertToRadians(360.0f * time) + 45.0f) * 2.0f);

		m_lightingConstants.lights[2].position.SetX(4.0f + sinf(XMConvertToRadians(360.0f * time)) * 2.0f);
		m_lightingConstants.lights[2].position.SetZ(0.0f + cosf(XMConvertToRadians(360.0f * time)) * 2.0f);

		m_lightingConstants.lights[4].position.SetX(0.0f + sinf(XMConvertToRadians(360.0f * time + 90.0f)) * 5.0f);
		m_lightingConstants.lights[4].position.SetZ(0.0f - cosf(XMConvertToRadians(360.0f * time + 45.0f)) * 5.0f);

		m_lightingConstants.lights[5].position.SetX(0.0f + sinf(XMConvertToRadians(-360.0f * time + 135.0f)) * 10.0f);
		m_lightingConstants.lights[5].position.SetZ(0.0f - cosf(XMConvertToRadians(-360.0f * time - 45.0f)) * 10.0f);
	}

	// Current view position
	m_lightingConstants.viewPosition = Vector4(m_camera.GetPosition(), 0.0f);

	m_lightingConstants.displayBuffer = m_displayBuffer;

	m_lightingConstantBuffer->Update(sizeof(LightingConstants), &m_lightingConstants);
}