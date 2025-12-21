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

#include "ShadowMappingApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"

using namespace Luna;
using namespace Math;
using namespace std;


ShadowMappingApp::ShadowMappingApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{}


int ShadowMappingApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void ShadowMappingApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void ShadowMappingApp::Startup()
{
	// Application initialization, after device creation
}


void ShadowMappingApp::Shutdown()
{
	// Application cleanup on shutdown
}


void ShadowMappingApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffers();
}


void ShadowMappingApp::UpdateUI()
{
	if (m_uiOverlay->Header("Settings")) 
	{
		m_uiOverlay->ComboBox("Scenes", &m_sceneIndex, m_sceneNames);
		m_uiOverlay->CheckBox("Display shadow render target", &m_visualizeShadowMap);
		m_uiOverlay->CheckBox("PCF filtering", &m_usePCF);
	}
}


void ShadowMappingApp::Render()
{
	auto& context = GraphicsContext::Begin("Frame");

	// Shadow depths
	{
		ScopedDrawEvent event(context, "Shadow Depths");

		context.TransitionResource(m_shadowMap, ResourceState::DepthWrite);
		context.ClearDepth(m_shadowMap);

		context.BeginRendering(m_shadowMap);

		context.SetViewportAndScissor(0u, 0u, m_shadowMapSize, m_shadowMapSize);

		context.SetRootSignature(m_shadowDepthRootSignature);
		context.SetGraphicsPipeline(m_shadowDepthPipeline);

		context.SetDepthBias(1.25f, 0.0f, 1.75f);

		context.SetRootCBV(0, m_shadowConstantBuffer);

		// Render current scene
		m_scenes[m_sceneIndex]->Render(context, true /* bPositionOnly */);

		context.EndRendering();
	}

	// Draw main scene (or visualize shadow map)
	{
		context.TransitionResource(m_shadowMap, ResourceState::PixelShaderResource);
		context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
		context.TransitionResource(GetDepthBuffer(), ResourceState::DepthWrite);
		context.ClearColor(GetColorBuffer());
		context.ClearDepth(GetDepthBuffer());

		context.BeginRendering(GetColorBuffer(), GetDepthBuffer());

		context.SetViewport(0.0f, 0.0f, (float)GetWindowWidth(), (float)GetWindowHeight());

		if (m_visualizeShadowMap)
		{
			ScopedDrawEvent event(context, "Visualize Shadow Map");

			context.SetRootSignature(m_shadowVisualizationRootSignature);
			context.SetGraphicsPipeline(m_shadowVisualizationPipeline);

			context.SetDescriptors(0, m_shadowVisualizationDescriptorSet);

			context.Draw(3);
		}
		else
		{
			ScopedDrawEvent event(context, "Scene");

			context.SetRootSignature(m_sceneRootSignature);
			context.SetGraphicsPipeline(m_scenePipeline);

			context.SetRootCBV(0, m_sceneConstantBuffer);
			context.SetDescriptors(1, m_scenePsDescriptorSet);

			m_scenes[m_sceneIndex]->Render(context);
		}
	}

	RenderUI(context);

	context.EndRendering();

	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void ShadowMappingApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		XMConvertToRadians(45.0f),
		GetWindowAspectRatio(),
		0.1f,
		256.0f);
	m_camera.SetPosition(Vector3(3.0f, 7.0f, 16.0f));
	

	InitShadowMap();
	InitConstantBuffers();
	InitRootSignatures();
	InitDescriptorSets();

	LoadAssets();

	m_controller.SetSpeedScale(0.01f);
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.RefreshFromCamera();
	m_controller.SetOrbitTarget(m_scenes[m_sceneIndex]->boundingBox.GetCenter(), Length(m_camera.GetPosition()), 4.0f);
}


void ShadowMappingApp::CreateWindowSizeDependentResources()
{
	if (!m_pipelinesCreated)
	{
		InitPipelines();
		m_pipelinesCreated = true;
	}

	m_camera.SetPerspectiveMatrix(
		XMConvertToRadians(45.0f),
		GetWindowAspectRatio(),
		0.1f,
		256.0f);
}


void ShadowMappingApp::InitRootSignatures()
{
	RootSignatureDesc shadowDepthDesc{
		.name = "Shadow Depth Root Signature",
		.rootParameters = { RootCBV(0, ShaderStage::Vertex) }
	};
	m_shadowDepthRootSignature = CreateRootSignature(shadowDepthDesc);

	RootSignatureDesc sceneDesc{
		.name = "Scene Root Signature",
		.rootParameters = { 
			RootCBV(0, ShaderStage::Vertex),
			Table({ TextureSRV }, ShaderStage::Pixel)
		},
		.staticSamplers = { StaticSampler(CommonStates::SamplerLinearClamp()) }
	};
	m_sceneRootSignature = CreateRootSignature(sceneDesc);

	RootSignatureDesc visualizationDesc{
		.name = "Shadow Visualization Root Signature",
		.rootParameters = {
			Table({ TextureSRV, ConstantBuffer }, ShaderStage::Pixel)
		},
		.staticSamplers = { StaticSampler(CommonStates::SamplerPointClamp()) }
	};
	m_shadowVisualizationRootSignature = CreateRootSignature(visualizationDesc);
}


void ShadowMappingApp::InitPipelines()
{
	// Shadow depth pipeline
	{
		auto vertexLayout = VertexLayout<VertexComponent::Position>();

		VertexStreamDesc vertexStreamDesc{
			.inputSlot				= 0,
			.stride					= vertexLayout.GetSizeInBytes(),
			.inputClassification	= InputClassification::PerVertexData
		};

		// Rasterizer state with depth-bias
		RasterizerStateDesc rasterizerStateDesc = CommonStates::RasterizerTwoSided();
		rasterizerStateDesc.depthBias = 1.25f;
		rasterizerStateDesc.slopeScaledDepthBias = 1.75f;

		GraphicsPipelineDesc desc{
			.name				= "Shadow Depth Graphics PSO",
			.blendState			= CommonStates::BlendDisable(),
			.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
			.rasterizerState	= rasterizerStateDesc,
			.dsvFormat			= Format::D16,
			.topology			= PrimitiveTopology::TriangleList,
			.vertexShader		= { .shaderFile = "ShadowDepthVS" },
			.vertexStreams		= { vertexStreamDesc },
			.vertexElements		= vertexLayout.GetElements(),
			.rootSignature		= m_shadowDepthRootSignature
		};
		m_shadowDepthPipeline = CreateGraphicsPipeline(desc);
	}

	// Scene pipeline
	{
		auto vertexLayout = VertexLayout<VertexComponent::PositionNormalColorTexcoord>();

		VertexStreamDesc vertexStreamDesc{
			.inputSlot				= 0,
			.stride					= vertexLayout.GetSizeInBytes(),
			.inputClassification	= InputClassification::PerVertexData
		};

		GraphicsPipelineDesc desc{
			.name				= "Scene Graphics PSO",
			.blendState			= CommonStates::BlendDisable(),
			.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
			.rasterizerState	= CommonStates::RasterizerDefault(),
			.rtvFormats			= { GetColorFormat() },
			.dsvFormat			= GetDepthFormat(),
			.topology			= PrimitiveTopology::TriangleList,
			.vertexShader		= { .shaderFile = "SceneVS" },
			.pixelShader		= { .shaderFile = "ScenePS" },
			.vertexStreams		= { vertexStreamDesc },
			.vertexElements		= vertexLayout.GetElements(),
			.rootSignature		= m_sceneRootSignature
		};
		m_scenePipeline = CreateGraphicsPipeline(desc);
	}

	// Shadow visualization pipeline
	{
		GraphicsPipelineDesc desc{
			.name				= "Shadow Visualization Graphics PSO",
			.blendState			= CommonStates::BlendDisable(),
			.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
			.rasterizerState	= CommonStates::RasterizerDefault(),
			.rtvFormats			= { GetColorFormat() },
			.dsvFormat			= GetDepthFormat(),
			.topology			= PrimitiveTopology::TriangleList,
			.vertexShader		= { .shaderFile = "ShadowVisualizationVS" },
			.pixelShader		= { .shaderFile = "ShadowVisualizationPS" },
			.rootSignature		= m_shadowVisualizationRootSignature
		};
		m_shadowVisualizationPipeline = CreateGraphicsPipeline(desc);
	}
}


void ShadowMappingApp::InitShadowMap()
{
	DepthBufferDesc desc{
		.name					= "Shadow Map",
		.width					= m_shadowMapSize,
		.height					= m_shadowMapSize,
		.format					= Format::D16,
		.createShaderResources	= true
	};
	m_shadowMap = CreateDepthBuffer(desc);
}


void ShadowMappingApp::InitConstantBuffers()
{
	m_shadowConstantBuffer = CreateConstantBuffer("Shadow Constant Buffer", 1, sizeof(ShadowConstants));
	m_sceneConstantBuffer = CreateConstantBuffer("Scene Constant Buffer", 1, sizeof(SceneConstants));
}


void ShadowMappingApp::InitDescriptorSets()
{
	m_scenePsDescriptorSet = m_sceneRootSignature->CreateDescriptorSet(1);
	m_scenePsDescriptorSet->SetSRV(0, m_shadowMap->GetSrvDescriptor(true));

	m_shadowVisualizationDescriptorSet = m_shadowVisualizationRootSignature->CreateDescriptorSet(0);
	m_shadowVisualizationDescriptorSet->SetSRV(0, m_shadowMap->GetSrvDescriptor(true));
	m_shadowVisualizationDescriptorSet->SetCBV(1, m_sceneConstantBuffer->GetCbvDescriptor());
}


void ShadowMappingApp::LoadAssets()
{
	auto layout = VertexLayout<VertexComponent::PositionNormalColorTexcoord>();
	m_scenes.resize(2);
	m_scenes[0] = LoadModel("vulkanscene_shadow.gltf", layout, 1.0f);
	m_scenes[1] = LoadModel("samplescene.gltf", layout, 1.0f);
	m_sceneNames = { "Vulkan Scene", "Teapots and Pillars" };
}


void ShadowMappingApp::UpdateConstantBuffers()
{
	// Animate light position
	const float seconds = 0.125f * (float)m_timer.GetTotalSeconds();
	m_lightPos.SetX(cosf(DirectX::XMConvertToRadians(seconds * 360.0f)) * 40.0f);
	m_lightPos.SetY(50.0f + sinf(DirectX::XMConvertToRadians(seconds * 360.0f)) * 20.0f);
	m_lightPos.SetZ(25.0f + sinf(DirectX::XMConvertToRadians(seconds * 360.0f)) * 5.0f);

	// Update shadow matrix
	Matrix4 depthProjectionMatrix = Matrix4::MakePerspective(DirectX::XMConvertToRadians(m_lightFOV), 1.0f, m_zNear, m_zFar);
	Matrix4 depthViewMatrix = Matrix4(XMMatrixLookAtRH(m_lightPos, Vector3(0.0f), Vector3(0.0f, 1.0f, 0.0f)));
	Matrix4 depthModelMatrix = Matrix4{ kIdentity };
	
	m_shadowConstants.modelViewProjectionMatrix = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;

	m_shadowConstantBuffer->Update(sizeof(ShadowConstants), &m_shadowConstants);

	// Update scene data
	m_sceneConstants.projectionMatrix = m_camera.GetProjectionMatrix();
	m_sceneConstants.viewMatrix = m_camera.GetViewMatrix();
	m_sceneConstants.modelMatrix = Matrix4{ kIdentity };
	m_sceneConstants.lightPosition = Vector4(m_lightPos, 1.0f);
	m_sceneConstants.depthBiasModelViewProjectionMatrix = m_shadowConstants.modelViewProjectionMatrix;
	m_sceneConstants.zNear = m_zNear;
	m_sceneConstants.zFar = m_zFar;

	m_sceneConstantBuffer->Update(sizeof(SceneConstants), &m_sceneConstants);
}