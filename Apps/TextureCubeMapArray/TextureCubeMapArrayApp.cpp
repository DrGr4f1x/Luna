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

#include "TextureCubeMapArrayApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"

using namespace Luna;
using namespace Math;
using namespace std;


TextureCubeMapArrayApp::TextureCubeMapArrayApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
	, m_controller{ m_camera, Vector3(kYUnitVector) }
{}


int TextureCubeMapArrayApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void TextureCubeMapArrayApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void TextureCubeMapArrayApp::Startup()
{
	// Application initialization, after device creation

	m_camera.SetPerspectiveMatrix(
		XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.001f,
		256.0f);
	m_camera.SetPosition(Vector3(0.0f, 0.0f, -4.0f));
	m_camera.Update();

	m_controller.SetSpeedScale(0.01f);
	m_controller.RefreshFromCamera();
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.SetOrbitTarget(Vector3(0.0f, 0.0f, 0.0f), 4.0f, 2.0f);
}


void TextureCubeMapArrayApp::Shutdown()
{
	// Application cleanup on shutdown
	m_models.clear();
}


void TextureCubeMapArrayApp::Update()
{
	// Application update tick
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffers();
}


void TextureCubeMapArrayApp::UpdateUI()
{
	if (m_uiOverlay->Header("Settings"))
	{
		m_uiOverlay->SliderInt("Cube map", &m_psConstants.arraySlice, 0, (int32_t)m_skyboxTex->GetArraySize() - 1);
		m_uiOverlay->SliderFloat("LOD bias", &m_psConstants.lodBias, 0.0f, (float)m_skyboxTex->GetNumMips());
		m_uiOverlay->ComboBox("Object type", &m_curModel, m_modelNames);
		m_uiOverlay->CheckBox("Skybox", &m_displaySkybox);
	}
}


void TextureCubeMapArrayApp::Render()
{
	// Application main render loop

	ScopedEvent event("Render");

	auto& context = GraphicsContext::Begin("Scene");

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(m_depthBuffer, ResourceState::DepthWrite);
	context.ClearColor(GetColorBuffer());
	context.ClearDepth(m_depthBuffer);

	context.BeginRendering(GetColorBuffer(), m_depthBuffer);

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());

	// Skybox
	if (m_displaySkybox)
	{
		ScopedDrawEvent skyBoxEvent(context, "Skybox");
		context.SetRootSignature(m_rootSignature);
		context.SetGraphicsPipeline(m_skyboxPipeline);

		context.SetResources(m_skyboxResources);

		m_skyboxModel->Render(context);
	}

	// Model
	{
		ScopedDrawEvent modelEvent(context, "Model");
		auto model = m_models[m_curModel];

		context.SetRootSignature(m_rootSignature);
		context.SetGraphicsPipeline(m_modelPipeline);

		context.SetResources(m_modelResources);

		model->Render(context);
	}

	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void TextureCubeMapArrayApp::CreateDeviceDependentResources()
{
	InitRootSignatures();
	InitConstantBuffers();
	LoadAssets();
	InitResourceSets();
}


void TextureCubeMapArrayApp::CreateWindowSizeDependentResources()
{
	InitDepthBuffer();
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
	m_camera.Update();
}


void TextureCubeMapArrayApp::InitDepthBuffer()
{
	DepthBufferDesc depthBufferDesc{
		.name			= "Depth Buffer",
		.resourceType	= ResourceType::Texture2D,
		.width			= GetWindowWidth(),
		.height			= GetWindowHeight(),
		.format			= GetDepthFormat()
	};

	m_depthBuffer = CreateDepthBuffer(depthBufferDesc);
}


void TextureCubeMapArrayApp::InitRootSignatures()
{
	auto rootSignatureDesc = RootSignatureDesc{
		.name = "Root Signature",
		.flags = RootSignatureFlags::AllowInputAssemblerInputLayout,
		.rootParameters =
			{
				RootCBV(0, ShaderStage::Vertex),
				Table({ ConstantBuffer, TextureSRV }, ShaderStage::Pixel),
				Table({ Sampler }, ShaderStage::Pixel)
			}
	};

	m_rootSignature = CreateRootSignature(rootSignatureDesc);
}


void TextureCubeMapArrayApp::InitPipelines()
{
	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= sizeof(Vertex),
		.inputClassification	= InputClassification::PerVertexData
	};

	vector<VertexElementDesc> vertexElements{
		{ "POSITION", 0, Format::RGB32_Float, 0, offsetof(Vertex, position), InputClassification::PerVertexData, 0 },
		{ "NORMAL", 0, Format::RGB32_Float, 0, offsetof(Vertex, normal), InputClassification::PerVertexData, 0 },
		{ "TEXCOORD", 0, Format::RG32_Float, 0, offsetof(Vertex, uv), InputClassification::PerVertexData, 0 }
	};

	GraphicsPipelineDesc skyBoxDesc
	{
		.name				= "Skybox Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadOnlyReversed(),
		.rasterizerState	= CommonStates::RasterizerDefaultCW(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= {.shaderFile = "SkyboxVS" },
		.pixelShader		= {.shaderFile = "SkyboxPS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= vertexElements,
		.rootSignature		= m_rootSignature
	};

	m_skyboxPipeline = CreateGraphicsPipeline(skyBoxDesc);

	GraphicsPipelineDesc modelDesc
	{
		.name				= "Model Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState	= CommonStates::RasterizerDefault(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= {.shaderFile = "ReflectVS" },
		.pixelShader		= {.shaderFile = "ReflectPS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= vertexElements,
		.rootSignature		= m_rootSignature
	};

	m_modelPipeline = CreateGraphicsPipeline(modelDesc);
}


void TextureCubeMapArrayApp::InitConstantBuffers()
{
	// Vertex shader cbuffer for the skybox
	GpuBufferDesc vsCbufferDesc{
		.name			= "VS Skybox Constant Buffer",
		.resourceType	= ResourceType::ConstantBuffer,
		.memoryAccess	= MemoryAccess::GpuRead | MemoryAccess::CpuWrite,
		.elementCount	= 1,
		.elementSize	= sizeof(VSConstants)
	};
	m_vsSkyboxConstantBuffer = CreateGpuBuffer(vsCbufferDesc);

	// Pixel shader cbuffer, shared
	GpuBufferDesc psCbufferDesc{
		.name			= "PS Skybox Constant Buffer",
		.resourceType	= ResourceType::ConstantBuffer,
		.memoryAccess	= MemoryAccess::GpuRead | MemoryAccess::CpuWrite,
		.elementCount	= 1,
		.elementSize	= sizeof(PSConstants)
	};
	m_psConstantBuffer = CreateGpuBuffer(psCbufferDesc);

	// Vertex shader cbuffer for the models
	m_vsModelConstantBuffer = CreateGpuBuffer(vsCbufferDesc);
}


void TextureCubeMapArrayApp::InitResourceSets()
{
	m_skyboxResources.Initialize(m_rootSignature);
	m_skyboxResources.SetCBV(0, 0, m_vsSkyboxConstantBuffer);
	m_skyboxResources.SetCBV(1, 0, m_psConstantBuffer);
	m_skyboxResources.SetSRV(1, 1, m_skyboxTex);
	m_skyboxResources.SetSampler(2, 0, m_sampler);

	m_modelResources.Initialize(m_rootSignature);
	m_modelResources.SetCBV(0, 0, m_vsModelConstantBuffer);
	m_modelResources.SetCBV(1, 0, m_psConstantBuffer);
	m_modelResources.SetSRV(1, 1, m_skyboxTex);
	m_modelResources.SetSampler(2, 0, m_sampler);
}

void TextureCubeMapArrayApp::UpdateConstantBuffers()
{
	Matrix4 modelMatrix = Matrix4(kIdentity);

	Matrix4 viewMatrix = AffineTransform(m_camera.GetOrientation(), Vector3(0.0f, 0.0f, 0.0f));

	m_vsSkyboxConstants.viewProjectionMatrix = m_camera.GetProjectionMatrix() * Invert(viewMatrix);
	m_vsSkyboxConstants.viewProjectionMatrix = m_camera.GetViewProjectionMatrix();
	m_vsSkyboxConstants.modelMatrix = modelMatrix;
	m_vsSkyboxConstants.eyePos = Vector3(0.0f, 0.0f, 0.0f);
	m_vsSkyboxConstantBuffer->Update(sizeof(m_vsSkyboxConstants), &m_vsSkyboxConstants);

	m_vsModelConstants.viewProjectionMatrix = m_camera.GetViewProjectionMatrix();
	m_vsModelConstants.modelMatrix = modelMatrix;
	m_vsModelConstants.eyePos = m_camera.GetPosition();
	m_vsModelConstantBuffer->Update(sizeof(m_vsModelConstants), &m_vsModelConstants);

	m_psConstantBuffer->Update(sizeof(m_psConstants), &m_psConstants);
}


void TextureCubeMapArrayApp::LoadAssets()
{
	m_skyboxTex = LoadTexture("cubemap_array.ktx");

	auto layout = VertexLayout<VertexComponent::PositionNormalTexcoord>();

	m_skyboxModel = LoadModel("cube.gltf", layout);

	auto model = LoadModel("sphere.gltf", layout);
	m_models.push_back(model);

	model = LoadModel("teapot.gltf", layout, 0.6f);
	m_models.push_back(model);

	model = LoadModel("torusknot.gltf", layout, 1.2f);
	m_models.push_back(model);

	model = LoadModel("venus.gltf", layout, 1.8f);
	m_models.push_back(model);

	m_modelNames.push_back("Sphere");
	m_modelNames.push_back("Teapot");
	m_modelNames.push_back("Torus knot");
	m_modelNames.push_back("Venus");

	m_sampler = CreateSampler(CommonStates::SamplerLinearClamp());
}