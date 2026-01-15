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

#include "SSAOApp.h"

#include "FileSystem.h"
#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"
#include "Graphics\Model.h"

using namespace Luna;
using namespace Math;
using namespace std;


SSAOApp::SSAOApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{
}


int SSAOApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void SSAOApp::Configure()
{
	// Application config, before device creation
	Application::Configure();

	m_fileSystem->AddSearchPath("..\\Data\\Models\\Sponza_VK");
}


void SSAOApp::Startup()
{
	// Application initialization, after device creation
}


void SSAOApp::Shutdown()
{
	// Application cleanup on shutdown
}


void SSAOApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffers();
}


void SSAOApp::UpdateUI()
{
	if (m_uiOverlay->Header("Settings"))
	{
		m_uiOverlay->CheckBox("Enable SSAO", &m_compositionConstants.ssao);
		m_uiOverlay->CheckBox("SSAO Blur", &m_compositionConstants.ssaoBlur);
		m_uiOverlay->CheckBox("SSAO Pass Only", &m_compositionConstants.ssaoOnly);
	}
}


void SSAOApp::Render()
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

		context.SetRootCBV(0, m_sceneConstantBuffer);

		uint32_t numMeshes = (uint32_t)m_model->meshes.size();
		for (uint32_t i = 0; i < numMeshes; ++i)
		{
#if APP_DYNAMIC_DESCRIPTORS
			context.SetSRV(1, 0, m_model->materials[i].diffuseTexture);
#else
			context.SetDescriptors(1, m_sceneDescriptorSets[i]);
#endif // APP_DYNAMIC_DESCRIPTORS

			m_model->meshes[i]->Render(context);
		}

		context.EndRendering();
	}

	// SSAO pass
	{
		ScopedDrawEvent offscreenEvent(context, "SSAO Generation Pass");

		context.TransitionResource(m_depthBuffer, ResourceState::PixelShaderResource);
		context.TransitionResource(m_ssaoRenderTarget, ResourceState::RenderTarget);
		context.TransitionResource(m_positionBuffer, ResourceState::PixelShaderResource);
		context.TransitionResource(m_normalBuffer, ResourceState::PixelShaderResource);
		context.ClearColor(m_ssaoRenderTarget);

		context.BeginRendering(m_ssaoRenderTarget);

		context.SetRootSignature(m_ssaoRootSignature);
		context.SetGraphicsPipeline(m_ssaoPipeline);

#if APP_DYNAMIC_DESCRIPTORS
		context.SetSRV(0, 0, m_positionBuffer);
		context.SetSRV(0, 1, m_depthBuffer);
		context.SetSRV(0, 2, m_normalBuffer);
		context.SetSRV(0, 3, m_noiseTexture);
		context.SetCBV(0, 0, m_ssaoKernelConstantBuffer);
		context.SetCBV(0, 1, m_ssaoConstantBuffer);
#else
		context.SetDescriptors(0, m_ssaoDescriptorSet);
#endif // APP_DYNAMIC_DESCRIPTORS

		context.Draw(3);

		context.EndRendering();
	}

	// SSAO blur pass
	{
		ScopedDrawEvent offscreenEvent(context, "SSAO Blur Pass");

		context.TransitionResource(m_ssaoRenderTarget, ResourceState::PixelShaderResource);
		context.TransitionResource(m_ssaoBlurRenderTarget, ResourceState::RenderTarget);
		context.ClearColor(m_ssaoBlurRenderTarget);

		context.BeginRendering(m_ssaoBlurRenderTarget);

		context.SetRootSignature(m_ssaoBlurRootSignature);
		context.SetGraphicsPipeline(m_ssaoBlurPipeline);

#if APP_DYNAMIC_DESCRIPTORS
		context.SetSRV(0, 0, m_ssaoRenderTarget);
#else
		context.SetDescriptors(0, m_ssaoBlurDescriptorSet);
#endif // APP_DYNAMIC_DESCRIPTORS

		context.Draw(3);

		context.EndRendering();
	}

	// Composite pass
	{
		ScopedDrawEvent offscreenEvent(context, "Composite Pass");

		context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
		context.TransitionResource(m_ssaoBlurRenderTarget, ResourceState::PixelShaderResource);
		context.TransitionResource(m_albedoBuffer, ResourceState::PixelShaderResource);
		context.ClearColor(GetColorBuffer());

		context.BeginRendering(GetColorBuffer());

		context.SetRootSignature(m_compositionRootSignature);
		context.SetGraphicsPipeline(m_compositionPipeline);

#if APP_DYNAMIC_DESCRIPTORS
		context.SetSRV(0, 0, m_positionBuffer);
		context.SetSRV(0, 1, m_normalBuffer);
		context.SetSRV(0, 2, m_albedoBuffer);
		context.SetSRV(0, 3, m_ssaoRenderTarget);
		context.SetSRV(0, 4, m_ssaoBlurRenderTarget);
		context.SetCBV(0, 0, m_compositionConstantBuffer);
#else
		context.SetDescriptors(0, m_compositionDescriptorSet);
#endif // APP_DYNAMIC_DESCRIPTORS

		context.Draw(3);
	}

	RenderUI(context);
	context.EndRendering();

	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void SSAOApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		m_nearPlane,
		m_farPlane);
	m_camera.SetPosition({ 0.0f, 0.75f, 1.0f });
	m_camera.SetOrientation(Quaternion(0.0f, DirectX::XMConvertToRadians(90.0f), 0.0f));

	m_controller.RefreshFromCamera();
	m_controller.SetCameraMode(CameraMode::WASD);
	m_controller.SlowMovement(true);
	m_controller.SlowRotation(false);
	m_controller.SetSpeedScale(0.025f);

	InitRootSignatures();
	InitConstantBuffers();

	LoadAssets();
	InitNoiseTexture();
}


void SSAOApp::CreateWindowSizeDependentResources()
{
	InitRenderTargets();

	if (!m_pipelinesCreated)
	{
		InitPipelines();
		m_pipelinesCreated = true;
	}

	m_camera.SetAspectRatio(GetWindowAspectRatio());

#if !APP_DYNAMIC_DESCRIPTORS
	InitDescriptorSets();
#endif // !APP_DYNAMIC_DESCRIPTORS
}


void SSAOApp::InitRenderTargets()
{
	const uint32_t width = GetWindowWidth();
	const uint32_t height = GetWindowHeight();

	// Position buffer
	ColorBufferDesc positionBufferDesc{
		.name		= "G-Buffer Position",
		.width		= width,
		.height		= height,
		.format		= Format::RGBA32_Float
	};
	m_positionBuffer = CreateColorBuffer(positionBufferDesc);

	// Normal buffer
	ColorBufferDesc normalBufferDesc{
		.name		= "G-Buffer Normals",
		.width		= width,
		.height		= height,
		.format		= Format::RGBA8_UNorm
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
		.name					= "G-Buffer Depth",
		.width					= width,
		.height					= height,
		.format					= GetDepthFormat(),
		.createShaderResources	= true
	};
	m_depthBuffer = CreateDepthBuffer(depthBufferDesc);

	// SSAO render target
	ColorBufferDesc ssaoRenderTargetDesc{
		.name	= "SSAO Render Target",
		.width	= width,
		.height = height,
		.format = Format::R8_UNorm
	};
	m_ssaoRenderTarget = CreateColorBuffer(ssaoRenderTargetDesc);

	// SSAO blur render target
	ColorBufferDesc ssaoBlurRenderTargetDesc{
		.name	= "SSAO Blur Render Target",
		.width	= width,
		.height = height,
		.format = Format::R8_UNorm
	};
	m_ssaoBlurRenderTarget = CreateColorBuffer(ssaoBlurRenderTargetDesc);
}


void SSAOApp::InitRootSignatures()
{
	// G-Buffer pass root signature
	RootSignatureDesc gbufferDesc{
		.name				= "G-Buffer Root Signature",
		.rootParameters		= {
			RootCBV(0, ShaderStage::Vertex | ShaderStage::Pixel),
			Table({TextureSRV}, ShaderStage::Pixel)
		},
		.staticSamplers		= {	StaticSampler(CommonStates::SamplerLinearWrap()) }
	};
	m_gbufferRootSignature = CreateRootSignature(gbufferDesc);

	// Composition pass root signature
	RootSignatureDesc compositionDesc{
		.name				= "Composition Root Signature",
		.rootParameters		= {
			Table({TextureSRV(0, 5), ConstantBuffer}, ShaderStage::Pixel)
		},
		.staticSamplers		= { StaticSampler(CommonStates::SamplerPointClamp()) }
	};
	m_compositionRootSignature = CreateRootSignature(compositionDesc);

	// SSAO generation pass root signature
	RootSignatureDesc ssaoGenerationDesc{
		.name				= "SSAO Generation Root Signature",
		.rootParameters		= {
			Table({TextureSRV(0, 4), ConstantBuffer(APPEND_REGISTER, 2)}, ShaderStage::Pixel)
		},
		.staticSamplers		= { StaticSampler(CommonStates::SamplerPointClamp()), StaticSampler(CommonStates::SamplerPointWrap()) }
	};
	m_ssaoRootSignature = CreateRootSignature(ssaoGenerationDesc);

	// SSAO blur pass root signature
	RootSignatureDesc ssaoBlurDesc{
		.name				= "SSAO Blur Root Signature",
		.rootParameters		= {
			Table({TextureSRV}, ShaderStage::Pixel)
		},
		.staticSamplers		= { StaticSampler(CommonStates::SamplerPointClamp()) }
	};
	m_ssaoBlurRootSignature = CreateRootSignature(ssaoBlurDesc);
}


void SSAOApp::InitPipelines()
{
	VertexStreamDesc vertexStreamDesc{
		.inputSlot = 0,
		.stride = sizeof(Vertex),
		.inputClassification = InputClassification::PerVertexData
	};

	vector<VertexElementDesc> vertexElements{
		{ "POSITION", 0, Format::RGB32_Float, 0, offsetof(Vertex, position), InputClassification::PerVertexData, 0 },
		{ "NORMAL", 0, Format::RGB32_Float, 0, offsetof(Vertex, normal), InputClassification::PerVertexData, 0 },
		{ "COLOR", 0, Format::RGBA32_Float, 0, offsetof(Vertex, color), InputClassification::PerVertexData, 0 },
		{ "TEXCOORD", 0, Format::RG32_Float, 0, offsetof(Vertex, uv), InputClassification::PerVertexData, 0 }
	};

	GraphicsPipelineDesc gbufferGraphicsPipelineDesc{
		.name				= "G-Buffer Graphics Pipeline",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState	= CommonStates::RasterizerDefault(),
		.rtvFormats			= { Format::RGBA32_Float, Format::RGBA8_UNorm, Format::RGBA8_UNorm },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "GBufferVS" },
		.pixelShader		= { .shaderFile = "GBufferPS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= vertexElements,
		.rootSignature		= m_gbufferRootSignature
	};
	m_gbufferPipeline = CreateGraphicsPipeline(gbufferGraphicsPipelineDesc);

	GraphicsPipelineDesc compositionGraphicsPipelineDesc{
		.name				= "Composition Graphics Pipeline",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateDisabled(),
		.rasterizerState	= CommonStates::RasterizerTwoSided(),
		.rtvFormats			= { GetColorFormat() },
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "FullscreenVS" },
		.pixelShader		= { .shaderFile = "CompositionPS" },
		.rootSignature		= m_compositionRootSignature
	};
	m_compositionPipeline = CreateGraphicsPipeline(compositionGraphicsPipelineDesc);

	GraphicsPipelineDesc ssaoGraphicsPipelineDesc{
		.name				= "SSAO Graphics Pipeline",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateDisabled(),
		.rasterizerState	= CommonStates::RasterizerTwoSided(),
		.rtvFormats			= { Format::R8_UNorm },
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "FullscreenVS" },
		.pixelShader		= { .shaderFile = "SSAOPS" },
		.rootSignature		= m_ssaoRootSignature
	};
	m_ssaoPipeline = CreateGraphicsPipeline(ssaoGraphicsPipelineDesc);

	GraphicsPipelineDesc ssaoBlurGraphicsPipelineDesc{
		.name				= "SSAO Blur Graphics Pipeline",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateDisabled(),
		.rasterizerState	= CommonStates::RasterizerTwoSided(),
		.rtvFormats			= { Format::R8_UNorm },
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "FullscreenVS" },
		.pixelShader		= { .shaderFile = "BlurPS" },
		.rootSignature		= m_ssaoBlurRootSignature
	};
	m_ssaoBlurPipeline = CreateGraphicsPipeline(ssaoBlurGraphicsPipelineDesc);
}


void SSAOApp::InitConstantBuffers()
{
	m_sceneConstantBuffer = CreateConstantBuffer("Scene Constant Buffer", 1, sizeof(SceneConstants));
	m_compositionConstantBuffer = CreateConstantBuffer("Composition Constant Buffer", 1, sizeof(CompositionConstants));
	m_ssaoConstantBuffer = CreateConstantBuffer("SSAO Constant Buffer", 1, sizeof(SSAOConstants));

	// Setup SSAO kernel
	for (uint32_t i = 0; i < SSAO_KERNEL_SIZE; ++i)
	{
		Vector3 sample{ 2.0f * g_rng.NextFloat() - 1.0f, 2.0f * g_rng.NextFloat() - 1.0f, g_rng.NextFloat() };
		sample = Normalize(sample);
		sample = g_rng.NextFloat() * sample;
		float scale = (float)i / (float)SSAO_KERNEL_SIZE;
		scale = Lerp(0.1f, 1.0f, scale * scale);
		m_ssaoKernelConstants.samples[i] = Vector4(scale * sample, 0.0f);
	}

	m_ssaoKernelConstantBuffer = CreateConstantBuffer("SSAO Kernel Constant Buffer", 1, sizeof(SSAOKernelConstants), &m_ssaoKernelConstants);
}


void SSAOApp::InitNoiseTexture()
{
	vector<Vector4> noiseValues(SSAO_NOISE_DIM * SSAO_NOISE_DIM);
	for (uint32_t i = 0; i < (uint32_t)noiseValues.size(); ++i)
	{
		noiseValues[i] = Vector4(2.0f * g_rng.NextFloat() - 1.0f, 2.0f * g_rng.NextFloat() - 1.0f, 0.0f, 0.0f);
	}

	TextureDesc textureDesc{
		.name		= "Noise Texture",
		.width		= SSAO_NOISE_DIM,
		.height		= SSAO_NOISE_DIM,
		.format		= Format::RGBA32_Float,
		.dataSize	= SSAO_NOISE_DIM * SSAO_NOISE_DIM * sizeof(Vector4),
		.data		= (std::byte*)noiseValues.data()
	};
	m_noiseTexture = CreateTexture2D(textureDesc);
}


#if !APP_DYNAMIC_DESCRIPTORS
void SSAOApp::InitDescriptorSets()
{
	for (uint32_t i = 0; i < (uint32_t)m_model->materials.size(); ++i)
	{
		auto descriptorSet = m_gbufferRootSignature->CreateDescriptorSet(1);
		descriptorSet->SetSRV(0, m_model->materials[i].diffuseTexture);
		m_sceneDescriptorSets.push_back(descriptorSet);
	}

	m_ssaoDescriptorSet = m_ssaoRootSignature->CreateDescriptorSet(0);
	m_ssaoDescriptorSet->SetSRV(0, m_positionBuffer);
	m_ssaoDescriptorSet->SetSRV(1, m_depthBuffer);
	m_ssaoDescriptorSet->SetSRV(2, m_normalBuffer);
	m_ssaoDescriptorSet->SetSRV(3, m_noiseTexture);
	m_ssaoDescriptorSet->SetCBV(0, m_ssaoKernelConstantBuffer);
	m_ssaoDescriptorSet->SetCBV(1, m_ssaoConstantBuffer);

	m_ssaoBlurDescriptorSet = m_ssaoBlurRootSignature->CreateDescriptorSet(0);
	m_ssaoBlurDescriptorSet->SetSRV(0, m_ssaoRenderTarget);

	m_compositionDescriptorSet = m_compositionRootSignature->CreateDescriptorSet(0);
	m_compositionDescriptorSet->SetSRV(0, m_positionBuffer);
	m_compositionDescriptorSet->SetSRV(1, m_normalBuffer);
	m_compositionDescriptorSet->SetSRV(2, m_albedoBuffer);
	m_compositionDescriptorSet->SetSRV(3, m_ssaoRenderTarget);
	m_compositionDescriptorSet->SetSRV(4, m_ssaoBlurRenderTarget);
	m_compositionDescriptorSet->SetCBV(0, m_compositionConstantBuffer);
}
#endif // !APP_DYNAMIC_DESCRIPTORS


void SSAOApp::LoadAssets()
{
	using enum VertexComponent;
	auto layout = VertexLayout<Position | Normal | Color0 | Texcoord0>();

	m_model = LoadModel("Sponza_VK/sponza.gltf", layout, 1.0f, Luna::ModelLoad::StandardDefault, true);
	assert(m_model->meshes.size() == m_model->materials.size());
}


void SSAOApp::UpdateConstantBuffers()
{
	m_sceneConstants.projectionMatrix = m_camera.GetProjectionMatrix();
	m_sceneConstants.modelMatrix = Matrix4{ kIdentity };
	m_sceneConstants.viewMatrix = m_camera.GetViewMatrix();
	m_sceneConstants.nearPlane = m_nearPlane;
	m_sceneConstants.farPlane = m_farPlane;

	m_ssaoConstants.projectionMatrix = m_camera.GetProjectionMatrix();
	m_ssaoConstants.gbufferTexDim[0] = (float)m_positionBuffer->GetWidth();
	m_ssaoConstants.gbufferTexDim[1] = (float)m_positionBuffer->GetHeight();
	m_ssaoConstants.noiseTexDim[0] = (float)m_noiseTexture->GetWidth();
	m_ssaoConstants.noiseTexDim[1] = (float)m_noiseTexture->GetHeight();
	m_ssaoConstants.invDepthRangeA = 1.0f;
	m_ssaoConstants.invDepthRangeB = 0.0f;
	m_ssaoConstants.linearizeDepthA = 1.0f / m_farPlane - 1.0f / m_nearPlane;
	m_ssaoConstants.linearizeDepthB = 1.0f / m_nearPlane;

	m_sceneConstantBuffer->Update(sizeof(m_sceneConstants), &m_sceneConstants);
	m_compositionConstantBuffer->Update(sizeof(m_compositionConstants), &m_compositionConstants);
	m_ssaoConstantBuffer->Update(sizeof(m_ssaoConstants), &m_ssaoConstants);
}