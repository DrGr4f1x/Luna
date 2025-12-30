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

#include "RadialBlurApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"

using namespace Luna;
using namespace Math;
using namespace std;


RadialBlurApp::RadialBlurApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{}


int RadialBlurApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void RadialBlurApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void RadialBlurApp::Startup()
{
	// Application initialization, after device creation
}


void RadialBlurApp::Shutdown()
{
	// Application cleanup on shutdown
}


void RadialBlurApp::Update()
{
	// Application update tick
	UpdateSceneConstantBuffer();
}


void RadialBlurApp::UpdateUI()
{
	if (m_uiOverlay->Header("Settings"))
	{
		m_uiOverlay->CheckBox("Radial blur", &m_blur);
		m_uiOverlay->CheckBox("Display render target", &m_displayTexture);

		if (m_blur) 
		{
			if (m_uiOverlay->Header("Blur parameters")) 
			{
				bool updateParams = false;
				updateParams |= m_uiOverlay->SliderFloat("Scale", &m_radialBlurConstants.radialBlurScale, 0.1f, 1.0f);
				updateParams |= m_uiOverlay->SliderFloat("Strength", &m_radialBlurConstants.radialBlurStrength, 0.1f, 2.0f);
				updateParams |= m_uiOverlay->SliderFloat("Horiz. origin", &m_radialBlurConstants.radialOrigin[0], 0.0f, 1.0f);
				updateParams |= m_uiOverlay->SliderFloat("Vert. origin", &m_radialBlurConstants.radialOrigin[1], 0.0f, 1.0f);
				if (updateParams) 
				{
					UpdateRadialBlurConstantBuffer();
				}
			}
		}
	}
}


void RadialBlurApp::Render()
{
	// Offscreen pass [offscreen render target]
	{
		auto& context = GraphicsContext::Begin("Offscreen Pass");

		context.TransitionResource(m_offscreenColorBuffer, ResourceState::RenderTarget);
		context.TransitionResource(m_offscreenDepthBuffer, ResourceState::DepthWrite);
		context.ClearColor(m_offscreenColorBuffer);
		context.ClearDepth(m_offscreenDepthBuffer);

		context.BeginRendering(m_offscreenColorBuffer, m_offscreenDepthBuffer);

		context.SetViewportAndScissor(0u, 0u, s_offscreenSize, s_offscreenSize);

		context.SetRootSignature(m_sceneRootSignature);
		context.SetGraphicsPipeline(m_colorPassPipeline);

		// Bind descriptor sets
		context.SetRootCBV(0, m_sceneConstantBuffer);
		context.SetDescriptors(1, m_sceneSrvDescriptorSet);

		// Draw the model
		m_model->Render(context);

		context.EndRendering();

		context.Finish();
	}

	// Main pass [backbuffer]
	{
		auto& context = GraphicsContext::Begin("Scene");

		context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
		context.TransitionResource(GetDepthBuffer(), ResourceState::DepthWrite);
		context.ClearColor(GetColorBuffer());
		context.ClearDepth(GetDepthBuffer());
		context.TransitionResource(m_offscreenColorBuffer, ResourceState::PixelShaderResource);

		context.BeginRendering(GetColorBuffer(), GetDepthBuffer());

		context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());

		context.SetRootSignature(m_sceneRootSignature);
		context.SetGraphicsPipeline(m_phongPassPipeline);

		// Bind descriptor sets
		context.SetRootCBV(0, m_sceneConstantBuffer);
		context.SetDescriptors(1, m_sceneSrvDescriptorSet);

		// Draw the model
		m_model->Render(context);

		if (m_blur)
		{
			context.SetRootSignature(m_radialBlurRootSignature);
			context.SetGraphicsPipeline(m_displayTexture ? m_displayTexturePipeline : m_radialBlurPipeline);

			// Bind descriptor sets
			context.SetDescriptors(0, m_blurCbvSrvDescriptorSet);

			context.Draw(3);
		}

		RenderUI(context);

		context.EndRendering();
		context.TransitionResource(GetColorBuffer(), ResourceState::Present);

		context.Finish();
	}
}


void RadialBlurApp::CreateDeviceDependentResources()
{
	InitRootSignatures();
	
	// Create and initialize constant buffers
	m_sceneConstantBuffer = CreateConstantBuffer("Scene Constant Buffer", 1, sizeof(SceneConstants));
	m_radialBlurConstantBuffer = CreateConstantBuffer("Radial Blur Constant Buffer", 1, sizeof(RadialBlurConstants));
	UpdateRadialBlurConstantBuffer();

	InitRenderTargets();
	LoadAssets();
	InitDescriptorSets();
}


void RadialBlurApp::CreateWindowSizeDependentResources()
{
	// Create any resources that depend on window size.  May be called when the window size changes.
	if (!m_pipelinesCreated)
	{
		InitPipelines();
		m_pipelinesCreated = true;
	}
}


void RadialBlurApp::InitRootSignatures()
{
	// Main scene
	auto sceneRootSignatureDesc = RootSignatureDesc{
		.name				= "Scene Root Signature",
		.rootParameters		= {
				RootCBV(0, ShaderStage::Vertex),
				Table({ TextureSRV }, ShaderStage::Pixel)
			},
		.staticSamplers		= { StaticSampler(CommonStates::SamplerLinearWrap()) }
	};

	m_sceneRootSignature = CreateRootSignature(sceneRootSignatureDesc);

	// Radial blur
	auto radialBlurRootSignatureDesc = RootSignatureDesc{
		.name				= "Radial Blur Root Signature",
		.rootParameters		= {
				Table({ TextureSRV, ConstantBuffer }, ShaderStage::Pixel)
			},
		.staticSamplers		= { StaticSampler(CommonStates::SamplerLinearClamp()) }
	};

	m_radialBlurRootSignature = CreateRootSignature(radialBlurRootSignatureDesc);
}


void RadialBlurApp::InitPipelines()
{
	auto vertexLayout = VertexLayout<VertexComponent::PositionNormalColorTexcoord>();

	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= vertexLayout.GetSizeInBytes(),
		.inputClassification	= InputClassification::PerVertexData
	};

	// Radial blur
	GraphicsPipelineDesc radialBlurPipelineDesc{
		.name				= "Radial Blur Graphics PSO",
		.blendState			= CommonStates::BlendAdditive(),
		.depthStencilState	= CommonStates::DepthStateReadOnlyReversed(),
		.rasterizerState	= CommonStates::RasterizerTwoSided(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "RadialBlurVS" },
		.pixelShader		= { .shaderFile = "RadialBlurPS" },
		.rootSignature		= m_radialBlurRootSignature
	};

	m_radialBlurPipeline = CreateGraphicsPipeline(radialBlurPipelineDesc);

	// Phong pass
	GraphicsPipelineDesc phongPassPipelineDesc{
		.name				= "Phong Pass Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState	= CommonStates::RasterizerDefault(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "PhongPassVS" },
		.pixelShader		= { .shaderFile = "PhongPassPS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= vertexLayout.GetElements(),
		.rootSignature		= m_sceneRootSignature
	};

	m_phongPassPipeline = CreateGraphicsPipeline(phongPassPipelineDesc);

	// Color pass
	GraphicsPipelineDesc colorPassPipelineDesc = phongPassPipelineDesc;
	colorPassPipelineDesc.SetName("Color Pass Graphics PSO");
	colorPassPipelineDesc.SetRtvFormats({ Format::RGBA8_UNorm });
	colorPassPipelineDesc.SetVertexShader("ColorPassVS");
	colorPassPipelineDesc.SetPixelShader("ColorPassPS");

	m_colorPassPipeline = CreateGraphicsPipeline(colorPassPipelineDesc);

	// Display texture pass
	GraphicsPipelineDesc displayTexturePipelineDesc = radialBlurPipelineDesc;
	displayTexturePipelineDesc.SetName("Display Texture Graphics PSO");
	displayTexturePipelineDesc.SetBlendState(CommonStates::BlendDisable());

	m_displayTexturePipeline = CreateGraphicsPipeline(displayTexturePipelineDesc);
}


void RadialBlurApp::InitRenderTargets()
{
	ColorBufferDesc colorBufferDesc{
		.name		= "Offscreen Color Buffer",
		.width		= s_offscreenSize,
		.height		= s_offscreenSize,
		.format		= Format::RGBA8_UNorm
	};

	m_offscreenColorBuffer = CreateColorBuffer(colorBufferDesc);

	DepthBufferDesc depthBufferDesc{
		.name		= "Offscreen Depth Buffer",
		.width		= s_offscreenSize,
		.height		= s_offscreenSize,
		.format		= GetDepthFormat()
	};

	m_offscreenDepthBuffer = CreateDepthBuffer(depthBufferDesc);
}


void RadialBlurApp::InitDescriptorSets()
{
	m_sceneSrvDescriptorSet = m_sceneRootSignature->CreateDescriptorSet(1);
	m_blurCbvSrvDescriptorSet = m_radialBlurRootSignature->CreateDescriptorSet(0);

	m_sceneSrvDescriptorSet->SetSRV(0, m_gradientTex);

	m_blurCbvSrvDescriptorSet->SetSRV(0, m_offscreenColorBuffer);
	m_blurCbvSrvDescriptorSet->SetCBV(0, m_radialBlurConstantBuffer);
}


void RadialBlurApp::LoadAssets()
{
	auto layout = VertexLayout<VertexComponent::PositionNormalColorTexcoord>();
	m_model = LoadModel("glowsphere.gltf", layout, 0.6f);
	m_gradientTex = LoadTexture("particle_gradient_rgba.ktx");
}


void RadialBlurApp::UpdateSceneConstantBuffer()
{
	m_sceneConstants.projectionMat = Matrix4::MakePerspective(
		DirectX::XMConvertToRadians(45.0f),
		(float)GetWindowWidth() / (float)GetWindowHeight(),
		0.1f,
		256.0f);

	Matrix4 viewMatrix = AffineTransform::MakeTranslation(Vector3(0.0f, 0.0f, m_zoom));

	auto rotation = AffineTransform::MakeXRotation(DirectX::XMConvertToRadians(m_rotation.GetX()));
	rotation = rotation * AffineTransform::MakeYRotation(DirectX::XMConvertToRadians(m_rotation.GetY()));
	rotation = rotation * AffineTransform::MakeYRotation(DirectX::XMConvertToRadians((float)m_timer.GetTotalSeconds() * 0.05f * 360.0f));
	rotation = rotation * AffineTransform::MakeZRotation(DirectX::XMConvertToRadians(m_rotation.GetZ()));

	m_sceneConstants.modelMat = Matrix4(kIdentity);
	m_sceneConstants.modelMat = Matrix4(AffineTransform::MakeTranslation(m_cameraPos)) * viewMatrix * rotation;

	if (m_isRunning)
	{
		m_sceneConstants.gradientPos += (float)m_timer.GetElapsedSeconds() * 0.1f;
	}

	m_sceneConstantBuffer->Update(sizeof(m_sceneConstants), &m_sceneConstants);
}


void RadialBlurApp::UpdateRadialBlurConstantBuffer()
{
	m_radialBlurConstantBuffer->Update(sizeof(RadialBlurConstants), &m_radialBlurConstants);
}