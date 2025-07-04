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

#include "TextureCubeMapApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"


using namespace Luna;
using namespace Math;
using namespace std;


TextureCubeMapApp::TextureCubeMapApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
	, m_controller{ m_camera, Vector3(kYUnitVector) }
{
}


int TextureCubeMapApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void TextureCubeMapApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void TextureCubeMapApp::Startup()
{
	// Application initialization, after device creation

	// TODO: Split this between CreateDeviceDependentResources() and CreateWindowSizeDependentResources

	m_camera.SetPerspectiveMatrix(
		XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.001f,
		256.0f);
	m_camera.SetPosition(Vector3(0.0f, 0.0f, 4.0f));

	m_camera.Update();

	m_controller.SetSpeedScale(0.01f);
	m_controller.RefreshFromCamera();
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.SetOrbitTarget(Vector3(0.0f, 0.0f, 0.0f), 4.0f, 2.0f);

	InitDepthBuffer();
	InitRootSignatures();
	InitPipelines();
	InitConstantBuffers();

	LoadAssets();

	InitResourceSets();
}


void TextureCubeMapApp::Shutdown()
{
	// Application cleanup on shutdown
	m_models.clear();
}


void TextureCubeMapApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffers();
}


void TextureCubeMapApp::UpdateUI()
{
	if (m_uiOverlay->Header("Settings"))
	{
		m_uiOverlay->SliderFloat("LOD bias", &m_psConstants.lodBias, 0.0f, (float)m_skyboxTex->GetNumMips());
		m_uiOverlay->ComboBox("Object type", &m_curModel, m_modelNames);
		m_uiOverlay->CheckBox("Skybox", &m_displaySkybox);
	}
}


void TextureCubeMapApp::Render()
{
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
		context.SetRootSignature(m_skyboxRootSignature);
		context.SetGraphicsPipeline(m_skyboxPipeline);

		context.SetResources(m_skyboxResources);

		m_skyboxModel->Render(context);
	}

	// Model
	{
		auto model = m_models[m_curModel];

		context.SetRootSignature(m_modelRootSignature);
		context.SetGraphicsPipeline(m_modelPipeline);

		context.SetResources(m_modelResources);

		model->Render(context);
	}

	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void TextureCubeMapApp::CreateDeviceDependentResources()
{
	// Create any resources that depend on the device, but not the window size
}


void TextureCubeMapApp::CreateWindowSizeDependentResources()
{
	// Create any resources that depend on window size.  May be called when the window size changes.
}


void TextureCubeMapApp::InitDepthBuffer()
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


void TextureCubeMapApp::InitRootSignatures()
{
	auto skyBoxRootSignatureDesc = RootSignatureDesc{
		.name	= "Skybox Root Signature",
		.flags	= RootSignatureFlags::AllowInputAssemblerInputLayout,
		.rootParameters =
			{
				RootParameter::RootCBV(0, ShaderStage::Vertex),
				RootParameter::Table({ DescriptorRange::TextureSRV(0) }, ShaderStage::Pixel),
				RootParameter::Table({ DescriptorRange::Sampler(0) }, ShaderStage::Pixel)
			}
	};

	m_skyboxRootSignature = CreateRootSignature(skyBoxRootSignatureDesc);

	auto modelRootSignatureDesc = RootSignatureDesc{
		.name	= "Model Root Signature",
		.flags	= RootSignatureFlags::AllowInputAssemblerInputLayout,
		.rootParameters =
			{
				RootParameter::RootCBV(0, ShaderStage::Vertex),
				RootParameter::Table({ DescriptorRange::ConstantBuffer(0), DescriptorRange::TextureSRV(1) }, ShaderStage::Pixel),
				RootParameter::Table({ DescriptorRange::Sampler(0) }, ShaderStage::Pixel)
			}
	};

	m_modelRootSignature = CreateRootSignature(modelRootSignatureDesc);
}


void TextureCubeMapApp::InitPipelines()
{
	VertexStreamDesc vertexStreamDesc{
		.inputSlot = 0,
		.stride = sizeof(Vertex),
		.inputClassification = InputClassification::PerVertexData
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
		.rootSignature		= m_skyboxRootSignature
	};

	m_skyboxPipeline = CreateGraphicsPipelineState(skyBoxDesc);

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
		.rootSignature		= m_modelRootSignature
	};

	m_modelPipeline = CreateGraphicsPipelineState(modelDesc);
}


void TextureCubeMapApp::InitConstantBuffers()
{
	// Vertex shader cbuffer for the skybox
	GpuBufferDesc vsSkyboxDesc{
		.name			= "VS Skybox Constant Buffer",
		.resourceType	= ResourceType::ConstantBuffer,
		.memoryAccess	= MemoryAccess::GpuRead | MemoryAccess::CpuWrite,
		.elementCount	= 1,
		.elementSize	= sizeof(VSConstants)
	};
	m_vsSkyboxConstantBuffer = CreateGpuBuffer(vsSkyboxDesc);
	
	// Vertex shader cbuffer for the models
	GpuBufferDesc vsModelDesc{
		.name			= "VS Model Constant Buffer",
		.resourceType	= ResourceType::ConstantBuffer,
		.memoryAccess	= MemoryAccess::GpuRead | MemoryAccess::CpuWrite,
		.elementCount	= 1,
		.elementSize	= sizeof(VSConstants)
	};
	m_vsModelConstantBuffer = CreateGpuBuffer(vsModelDesc);
	
	// Pixel shader cbuffer for the models
	GpuBufferDesc psModelDesc{
		.name			= "PS Model Constant Buffer",
		.resourceType	= ResourceType::ConstantBuffer,
		.memoryAccess	= MemoryAccess::GpuRead | MemoryAccess::CpuWrite,
		.elementCount	= 1,
		.elementSize	= sizeof(VSConstants)
	};
	m_psModelConstantBuffer = CreateGpuBuffer(psModelDesc);
}


void TextureCubeMapApp::InitResourceSets()
{
	m_skyboxResources.Initialize(m_skyboxRootSignature);
	m_skyboxResources.SetCBV(0, 0, m_vsSkyboxConstantBuffer);
	m_skyboxResources.SetSRV(1, 0, m_skyboxTex);
	m_skyboxResources.SetSampler(2, 0, m_sampler);

	m_modelResources.Initialize(m_modelRootSignature);
	m_modelResources.SetCBV(0, 0, m_vsModelConstantBuffer);
	m_modelResources.SetCBV(1, 0, m_psModelConstantBuffer);
	m_modelResources.SetSRV(1, 1, m_skyboxTex);
	m_modelResources.SetSampler(2, 0, m_sampler);
}

void TextureCubeMapApp::UpdateConstantBuffers()
{
	Matrix4 modelMatrix = Matrix4(kIdentity);

	Matrix4 viewMatrix = AffineTransform(m_camera.GetRotation(), Vector3(0.0f, 0.0f, 0.0f));

	m_vsSkyboxConstants.viewProjectionMatrix = m_camera.GetProjMatrix() * Invert(viewMatrix);
	m_vsSkyboxConstants.modelMatrix = modelMatrix;
	m_vsSkyboxConstants.eyePos = Vector3(0.0f, 0.0f, 0.0f);
	m_vsSkyboxConstantBuffer->Update(sizeof(m_vsSkyboxConstants), &m_vsSkyboxConstants);

	m_vsModelConstants.viewProjectionMatrix = m_camera.GetViewProjMatrix();
	m_vsModelConstants.modelMatrix = modelMatrix;
	m_vsModelConstants.eyePos = m_camera.GetPosition();
	m_vsModelConstantBuffer->Update(sizeof(m_vsModelConstants), &m_vsModelConstants);

	m_psModelConstantBuffer->Update(sizeof(m_psConstants), &m_psConstants);
}


void TextureCubeMapApp::LoadAssets()
{
	m_skyboxTex = LoadTexture("cubemap_yokohama_bc3_unorm.ktx");

	auto layout = VertexLayout<VertexComponent::PositionNormalTexcoord>();

	m_skyboxModel = LoadModel("cube.obj", layout, 0.05f);

	auto model = LoadModel("sphere.obj", layout, 0.05f);
	m_models.push_back(model);

	model = LoadModel("teapot.dae", layout, 0.05f);
	m_models.push_back(model);

	model = LoadModel("torusknot.obj", layout, 0.05f);
	m_models.push_back(model);

	//model = LoadModel("venus.fbx", layout, 0.15f);
	//m_models.push_back(model);

	m_modelNames.push_back("Sphere");
	m_modelNames.push_back("Teapot");
	m_modelNames.push_back("Torus knot");
	//m_modelNames.push_back("Venus");

	m_sampler = CreateSampler(CommonStates::SamplerLinearClamp());
}