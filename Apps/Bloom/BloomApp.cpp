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

#include "BloomApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"

using namespace Luna;
using namespace Math;
using namespace std;


BloomApp::BloomApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
	, m_controller{ m_camera, Vector3(kYUnitVector) }
{}


int BloomApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void BloomApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void BloomApp::Startup()
{
	// Application initialization, after device creation
}


void BloomApp::Shutdown()
{
	// Application cleanup on shutdown
}


void BloomApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffers();
}


void BloomApp::UpdateUI()
{
	if (m_uiOverlay->Header("Settings"))
	{
		m_uiOverlay->CheckBox("Bloom", &m_bloom);
		if (m_uiOverlay->InputFloat("Scale", &m_blurScale, 0.1f))
		{
			UpdateBlurConstants();
		}
	}
}


void BloomApp::Render()
{
	auto& context = GraphicsContext::Begin("Frame");

	// Offscreen color pass

	if (m_bloom)
	{
		ScopedDrawEvent offscreenEvent(context, "Offscreen pass");

		context.TransitionResource(m_offscreenColorBuffer[0], ResourceState::RenderTarget);
		context.TransitionResource(m_offscreenDepthBuffer, ResourceState::DepthWrite);
		context.ClearColor(m_offscreenColorBuffer[0]);
		context.ClearDepth(m_offscreenDepthBuffer);

		// 3D scene (glow pass)
		{
			ScopedDrawEvent event(context, "Glow pass");

			context.BeginRendering(m_offscreenColorBuffer[0], m_offscreenDepthBuffer);

			context.SetViewportAndScissor(0u, 0u, m_offscreenBufferSize, m_offscreenBufferSize);

			context.SetRootSignature(m_sceneRootSignature);
			context.SetGraphicsPipeline(m_colorPassPipeline);

			context.SetResources(m_sceneResources);

			// Render model
			{
				for (const auto& mesh : m_ufoGlowModel->meshes)
				{
					context.SetIndexBuffer(mesh->indexBuffer);
					context.SetVertexBuffer(0, mesh->vertexBuffer);

					for (const auto& meshPart : mesh->meshParts)
					{
						context.DrawIndexed(meshPart.indexCount, meshPart.indexBase, meshPart.vertexBase);
					}
				}
			}

			context.EndRendering();
		}

		// Vertical blur pass
		{
			ScopedDrawEvent event(context, "Vertical blur");

			context.TransitionResource(m_offscreenColorBuffer[0], ResourceState::PixelShaderResource);
			context.TransitionResource(m_offscreenColorBuffer[1], ResourceState::RenderTarget);
			context.ClearColor(m_offscreenColorBuffer[1]);

			context.BeginRendering(m_offscreenColorBuffer[1]);

			context.SetRootSignature(m_blurRootSignature);
			context.SetGraphicsPipeline(m_blurVertPipeline);

			context.SetResources(m_blurVertResources);
			context.Draw(3);

			context.EndRendering();
		}
	}
	else
	{
		context.TransitionResource(m_offscreenColorBuffer[1], ResourceState::RenderTarget);
		context.ClearColor(m_offscreenColorBuffer[1]);
	}

	// Backbuffer color pass

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(m_offscreenColorBuffer[1], ResourceState::PixelShaderResource);
	context.TransitionResource(m_depthBuffer, ResourceState::DepthWrite);
	context.ClearColor(GetColorBuffer());
	context.ClearDepth(m_depthBuffer);

	context.BeginRendering(GetColorBuffer(), m_depthBuffer);

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());

	// Skybox
	{
		ScopedDrawEvent event(context, "Skybox");

		context.SetRootSignature(m_skyboxRootSignature);
		context.SetGraphicsPipeline(m_skyboxPipeline);

		context.SetResources(m_skyboxResources);

		// Render model
		{
			for (const auto& mesh : m_skyboxModel->meshes)
			{
				context.SetIndexBuffer(mesh->indexBuffer);
				context.SetVertexBuffer(0, mesh->vertexBuffer);

				for (const auto& meshPart : mesh->meshParts)
				{
					context.DrawIndexed(meshPart.indexCount, meshPart.indexBase, meshPart.vertexBase);
				}
			}
		}
	}

	// 3D scene (phong pass)
	{
		ScopedDrawEvent event(context, "Phong pass");

		context.SetRootSignature(m_sceneRootSignature);
		context.SetGraphicsPipeline(m_phongPassPipeline);

		context.SetResources(m_sceneResources);

		// Render model
		{
			for (const auto& mesh : m_ufoGlowModel->meshes)
			{
				context.SetIndexBuffer(mesh->indexBuffer);
				context.SetVertexBuffer(0, mesh->vertexBuffer);

				for (const auto& meshPart : mesh->meshParts)
				{
					context.DrawIndexed(meshPart.indexCount, meshPart.indexBase, meshPart.vertexBase);
				}
			}
		}
	}

	// Horizontal blur pass
	{
		ScopedDrawEvent event(context, "Horizontal blur");

		context.SetRootSignature(m_blurRootSignature);
		context.SetGraphicsPipeline(m_blurHorizPipeline);

		context.SetResources(m_blurHorizResources);
		context.Draw(3);
	}

	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void BloomApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		XMConvertToRadians(45.0f),
		GetWindowAspectRatio(),
		0.1f,
		256.0f);
	m_camera.SetPosition(Vector3(0.0f, 0.0f, 10.25f));
	m_camera.Update();
	m_camera.SetOrientation(Quaternion(XMConvertToRadians(7.5f), XMConvertToRadians(-343.0f), 0.0f));

	m_controller.SetSpeedScale(0.01f);
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.RefreshFromCamera();
	m_controller.SetOrbitTarget(Vector3(0.0f, -2.0f, 0.0f), Length(m_camera.GetPosition()), 4.0f);

	InitOffscreenBuffers();
	InitRootSignatures();
	InitConstantBuffers();

	LoadAssets();

	InitResourceSets();
}


void BloomApp::CreateWindowSizeDependentResources()
{
	// Create any resources that depend on window size.  May be called when the window size changes.
	InitDepthBuffer();
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
	m_camera.Update();
}


void BloomApp::InitDepthBuffer()
{
	DepthBufferDesc depthBufferDesc{
		.name		= "Depth Buffer",
		.width		= GetWindowWidth(),
		.height		= GetWindowHeight(),
		.format		= GetDepthFormat()
	};

	m_depthBuffer = CreateDepthBuffer(depthBufferDesc);
}


void BloomApp::InitOffscreenBuffers()
{
	for (uint32_t i = 0; i < 2; ++i)
	{
		ColorBufferDesc colorBufferDesc{
			.name		= format("Offscreen ColorBuffer {}", i),
			.width		= m_offscreenBufferSize,
			.height		= m_offscreenBufferSize,
			.format		= Format::RGBA8_UNorm
		};
		m_offscreenColorBuffer[i] = CreateColorBuffer(colorBufferDesc);
	}

	DepthBufferDesc depthBufferDesc{
		.name		= "Offscreen Depth Buffer",
		.width		= m_offscreenBufferSize,
		.height		= m_offscreenBufferSize,
		.format		= GetDepthFormat()
	};

	m_offscreenDepthBuffer = CreateDepthBuffer(depthBufferDesc);
}


void BloomApp::InitRootSignatures()
{
	// Scene root signature
	RootSignatureDesc sceneRootSignatureDesc{
		.name				= "Scene Root Signature",
		.flags				= RootSignatureFlags::AllowInputAssemblerInputLayout | RootSignatureFlags::DenyPixelShaderRootAccess,
		.rootParameters		= {	RootCBV(0, ShaderStage::Vertex) }
	};
	m_sceneRootSignature = CreateRootSignature(sceneRootSignatureDesc);

	// Blur root signature
	RootSignatureDesc blurRootSignatureDesc{
		.name				= "Blur Root Signature",
		.flags				= RootSignatureFlags::AllowInputAssemblerInputLayout | RootSignatureFlags::DenyVertexShaderRootAccess,
		.rootParameters		= {	
			Table({ TextureSRV, ConstantBuffer }, ShaderStage::Pixel),
			Table({ Sampler }, ShaderStage::Pixel)
		}
	};
	m_blurRootSignature = CreateRootSignature(blurRootSignatureDesc);

	// Skybox root signature
	RootSignatureDesc skyboxRootSignatureDesc{
		.name				= "Skybox Root Signature",
		.flags				= RootSignatureFlags::AllowInputAssemblerInputLayout,
		.rootParameters		= {
			Table({ ConstantBuffer }, ShaderStage::Vertex),
			Table({ TextureSRV }, ShaderStage::Pixel),
			Table({ Sampler }, ShaderStage::Pixel)
		}
	};
	m_skyboxRootSignature = CreateRootSignature(skyboxRootSignatureDesc);
}


void BloomApp::InitPipelines()
{
	auto vertexLayout = VertexLayout<VertexComponent::PositionNormalColorTexcoord>();

	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= vertexLayout.GetSizeInBytes(),
		.inputClassification	= InputClassification::PerVertexData
	};

	// Color pass pipeline
	GraphicsPipelineDesc colorPassPipelineDesc{
		.name				= "Color Pass Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState	= CommonStates::RasterizerTwoSided(),
		.rtvFormats			= { Format::RGBA8_UNorm },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "ColorPassVS" },
		.pixelShader		= { .shaderFile = "ColorPassPS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= vertexLayout.GetElements(),
		.rootSignature		= m_sceneRootSignature
	};
	m_colorPassPipeline = CreateGraphicsPipeline(colorPassPipelineDesc);

	// Phong pass pipeline
	GraphicsPipelineDesc phongPassPipelineDesc{
		.name				= "Phong Pass Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState	= CommonStates::RasterizerTwoSided(),
		.rtvFormats			= { GetColorFormat()},
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "PhongPassVS" },
		.pixelShader		= { .shaderFile = "PhongPassPS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= vertexLayout.GetElements(),
		.rootSignature		= m_sceneRootSignature
	};
	m_phongPassPipeline = CreateGraphicsPipeline(phongPassPipelineDesc);

	// Vertical blur pipeline
	GraphicsPipelineDesc verticalBlurPipelineDesc{
		.name				= "Vertical Blur Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateDisabled(),
		.rasterizerState	= CommonStates::RasterizerTwoSided(),
		.rtvFormats			= { Format::RGBA8_UNorm },
		.dsvFormat			= Format::Unknown,
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "GaussianBlurVS" },
		.pixelShader		= { .shaderFile = "GaussianBlurPS" },
		.rootSignature		= m_blurRootSignature
	};
	m_blurVertPipeline = CreateGraphicsPipeline(verticalBlurPipelineDesc);

	// Horizontal blur pipeline
	GraphicsPipelineDesc horizontalBlurPipelineDesc{
		.name				= "Horizontal Blur Graphics PSO",
		.blendState			= CommonStates::BlendAdditive(),
		.depthStencilState	= CommonStates::DepthStateDisabled(),
		.rasterizerState	= CommonStates::RasterizerTwoSided(),
		.rtvFormats			= { Format::RGBA8_UNorm },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= {.shaderFile = "GaussianBlurVS" },
		.pixelShader		= {.shaderFile = "GaussianBlurPS" },
		.rootSignature		= m_blurRootSignature
	};
	m_blurHorizPipeline = CreateGraphicsPipeline(horizontalBlurPipelineDesc);

	// Skybox pipeline
	GraphicsPipelineDesc skyboxPipelineDesc{
		.name				= "Skybox Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateDisabled(),
		.rasterizerState	= CommonStates::RasterizerTwoSided(),
		.rtvFormats			= { GetColorFormat()},
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "SkyboxVS" },
		.pixelShader		= { .shaderFile = "SkyboxPS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= vertexLayout.GetElements(),
		.rootSignature		= m_skyboxRootSignature
	};
	m_skyboxPipeline = CreateGraphicsPipeline(skyboxPipelineDesc);
}


void BloomApp::InitConstantBuffers()
{
	// Scene constant buffer
	GpuBufferDesc sceneConstantBufferDesc{
		.name			= "Scene Constant Buffer",
		.resourceType	= ResourceType::ConstantBuffer,
		.memoryAccess	= MemoryAccess::GpuRead | MemoryAccess::CpuWrite,
		.elementCount	= 1,
		.elementSize	= sizeof(SceneConstants)
	};
	m_sceneConstantBuffer = CreateGpuBuffer(sceneConstantBufferDesc);

	// Skybox constant buffer
	GpuBufferDesc skyboxConstantBufferDesc = sceneConstantBufferDesc;
	skyboxConstantBufferDesc.SetName("Skybox Constant Buffer");
	m_skyboxConstantBuffer = CreateGpuBuffer(skyboxConstantBufferDesc);

	// Vertical blur constant buffer
	m_blurVertConstants.blurScale = m_blurScale;
	m_blurVertConstants.blurStrength = 1.5f;
	m_blurVertConstants.blurDirection = 0;

	GpuBufferDesc verticalBlurConstantBufferDesc{
		.name			= "Vertical Blur Constant Buffer",
		.resourceType	= ResourceType::ConstantBuffer,
		.memoryAccess	= MemoryAccess::GpuRead | MemoryAccess::CpuWrite,
		.elementCount	= 1,
		.elementSize	= sizeof(BlurConstants),
		.initialData	= &m_blurVertConstants
	};
	m_blurVertConstantBuffer = CreateGpuBuffer(verticalBlurConstantBufferDesc);

	// Horizontal blur constant buffer
	m_blurHorizConstants.blurScale = m_blurScale;
	m_blurHorizConstants.blurStrength = 1.5f;
	m_blurHorizConstants.blurDirection = 1;

	GpuBufferDesc horizontalBlurConstantBufferDesc{
		.name			= "Horizontal Blur Constant Buffer",
		.resourceType	= ResourceType::ConstantBuffer,
		.memoryAccess	= MemoryAccess::GpuRead | MemoryAccess::CpuWrite,
		.elementCount	= 1,
		.elementSize	= sizeof(BlurConstants),
		.initialData	= &m_blurHorizConstants
	};
	m_blurHorizConstantBuffer = CreateGpuBuffer(horizontalBlurConstantBufferDesc);
}


void BloomApp::InitResourceSets()
{
	m_sceneResources.Initialize(m_sceneRootSignature);
	m_sceneResources.SetCBV(0, 0, m_sceneConstantBuffer);

	m_skyboxResources.Initialize(m_skyboxRootSignature);
	m_skyboxResources.SetCBV(0, 0, m_skyboxConstantBuffer);
	m_skyboxResources.SetSRV(1, 0, m_skyboxTexture);
	m_skyboxResources.SetSampler(2, 0, m_sampler);

	m_blurHorizResources.Initialize(m_blurRootSignature);
	m_blurHorizResources.SetSRV(0, 0, m_offscreenColorBuffer[1]);
	m_blurHorizResources.SetCBV(0, 1, m_blurHorizConstantBuffer);
	m_blurHorizResources.SetSampler(1, 0, m_sampler);

	m_blurVertResources.Initialize(m_blurRootSignature);
	m_blurVertResources.SetSRV(0, 0, m_offscreenColorBuffer[0]);
	m_blurVertResources.SetCBV(0, 1, m_blurVertConstantBuffer);
	m_blurVertResources.SetSampler(1, 0, m_sampler);
}


void BloomApp::LoadAssets()
{
	auto layout = VertexLayout<VertexComponent::PositionNormalColorTexcoord>();
	m_ufoModel = LoadModel("retroufo.gltf", layout, 1.0f);
	m_ufoGlowModel = LoadModel("retroufo_glow.gltf", layout, 1.0f);
	m_skyboxModel = LoadModel("cube.gltf", layout, 1.0f);
	m_skyboxTexture = LoadTexture("cubemap_space.ktx");
	m_sampler = CreateSampler(CommonStates::SamplerLinearClamp());
}


void BloomApp::UpdateConstantBuffers()
{
	m_sceneConstants.projectionMat = m_camera.GetProjectionMatrix();
	m_sceneConstants.viewMat = m_camera.GetViewMatrix();
	m_sceneConstants.modelMat = Matrix4(kIdentity);

	AffineTransform modelTrans{ kIdentity };
	float time = (float)m_timer.GetTotalSeconds() * 0.125f;
	modelTrans = AffineTransform::MakeTranslation(Vector3(sinf(XMConvertToRadians(time * 360.0f)) * 0.25f, -2.0f, cosf(XMConvertToRadians(time * 360.0f)) * 0.25f));
	modelTrans = modelTrans * AffineTransform::MakeXRotation(-sinf(XMConvertToRadians(time * 360.0f)) * 0.15f);
	modelTrans = modelTrans * AffineTransform::MakeYRotation(XMConvertToRadians(time * 360.0f));
	m_sceneConstants.modelMat = modelTrans;

	m_skyboxConstants.projectionMat = Matrix4::MakePerspective(XMConvertToRadians(45.0f), (float)GetWindowWidth() / (float)GetWindowHeight(), 0.1f, 256.0f);
	m_skyboxConstants.viewMat = Matrix4(Matrix3(m_sceneConstants.viewMat));
	m_skyboxConstants.modelMat = Matrix4(kIdentity);

	m_sceneConstantBuffer->Update(sizeof(SceneConstants), &m_sceneConstants);
	m_skyboxConstantBuffer->Update(sizeof(SceneConstants), &m_skyboxConstants);
}


void BloomApp::UpdateBlurConstants()
{
	m_blurHorizConstants.blurScale = m_blurScale;
	m_blurVertConstants.blurScale = m_blurScale;

	m_blurHorizConstantBuffer->Update(sizeof(BlurConstants), &m_blurHorizConstants);
	m_blurVertConstantBuffer->Update(sizeof(BlurConstants), &m_blurVertConstants);
}