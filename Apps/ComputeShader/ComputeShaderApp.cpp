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

#include "ComputeShaderApp.h"

#include "InputSystem.h"
#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"

using namespace Luna;
using namespace Math;
using namespace std;


ComputeShaderApp::ComputeShaderApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{}


int ComputeShaderApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void ComputeShaderApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void ComputeShaderApp::Startup()
{
	// Application initialization, after device creation

	// Setup camera
	m_camera.SetPerspectiveMatrix(
		XMConvertToRadians(60.0f),
		2.0f * GetWindowAspectRatio(),
		0.001f,
		256.0f);
	m_camera.SetPosition(Vector3(0.0f, 0.0f, 2.0f));

	m_shaderNames.push_back("Emboss");
	m_shaderNames.push_back("Edge Detect");
	m_shaderNames.push_back("Sharpen");
}


void ComputeShaderApp::Shutdown()
{
	// Application cleanup on shutdown
}


void ComputeShaderApp::Update()
{
	// Application update tick
	// Set m_bIsRunning to false if your application wants to exit

	if (m_inputSystem->IsFirstPressed(DigitalInput::kKey_add))
	{
		++m_curComputeTechnique;
	}
	else if (m_inputSystem->IsFirstPressed(DigitalInput::kKey_subtract))
	{
		--m_curComputeTechnique;
	}

	if (m_curComputeTechnique < 0)
	{
		m_curComputeTechnique = 2;
	}

	if (m_curComputeTechnique > 2)
	{
		m_curComputeTechnique = 0;
	}
}


void ComputeShaderApp::UpdateUI()
{
	if (m_uiOverlay->Header("Settings"))
	{
		m_uiOverlay->ComboBox("Shader", &m_curComputeTechnique, m_shaderNames);
	}
}


void ComputeShaderApp::Render()
{
	// Application main render loop
	
	auto& context = GraphicsContext::Begin("Scene");

	// Compute
	{
		auto& computeContext = context.GetComputeContext();

		computeContext.TransitionResource(m_computeScratchBuffer, ResourceState::UnorderedAccess);
		computeContext.TransitionResource(m_texture, ResourceState::NonPixelShaderResource);

		computeContext.SetRootSignature(m_computeRootSignature);
		if (m_curComputeTechnique == 0)
		{
			computeContext.SetComputePipeline(m_embossPipeline);
		}
		else if (m_curComputeTechnique == 1)
		{
			computeContext.SetComputePipeline(m_edgeDetectPipeline);
		}
		else
		{
			computeContext.SetComputePipeline(m_sharpenPipeline);
		}

		computeContext.SetDescriptors(0, m_computeSrvUavDescriptorSet);

		computeContext.Dispatch2D((uint32_t)m_computeScratchBuffer->GetWidth(), m_computeScratchBuffer->GetHeight(), 16, 16);

		computeContext.TransitionResource(m_computeScratchBuffer, ResourceState::PixelShaderResource);
	}

	// Graphics

	context.TransitionResource(m_texture, ResourceState::PixelShaderResource, true);
	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(GetDepthBuffer(), ResourceState::DepthWrite);
	context.ClearColor(GetColorBuffer());
	context.ClearDepth(GetDepthBuffer());

	context.BeginRendering(GetColorBuffer(), GetDepthBuffer());

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth() / 2, GetWindowHeight());

	context.SetRootSignature(m_graphicsRootSignature);
	context.SetGraphicsPipeline(m_graphicsPipeline);

	context.SetDescriptors(0, m_graphicsLeftCbvDescriptorSet);
	context.SetDescriptors(1, m_graphicsLeftSrvDescriptorSet);

	context.SetVertexBuffer(0, m_vertexBuffer);
	context.SetIndexBuffer(m_indexBuffer);

	context.DrawIndexed((uint32_t)m_indexBuffer->GetElementCount());

	context.SetViewportAndScissor(GetWindowWidth() / 2, 0u, GetWindowWidth() / 2, GetWindowHeight());

	context.SetDescriptors(0, m_graphicsRightCbvDescriptorSet);
	context.SetDescriptors(1, m_graphicsRightSrvDescriptorSet);

	context.SetVertexBuffer(0, m_vertexBuffer);
	context.SetIndexBuffer(m_indexBuffer);

	context.DrawIndexed((uint32_t)m_indexBuffer->GetElementCount());

	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void ComputeShaderApp::CreateDeviceDependentResources()
{
	// Create any resources that depend on the device, but not the window size

	// Setup vertices for a single uv-mapped quad made from two triangles
	vector<Vertex> vertexData =
	{
		{ {  1.0f,  1.0f,  0.0f }, { 1.0f, 0.0f } },
		{ { -1.0f,  1.0f,  0.0f }, { 0.0f, 0.0f } },
		{ { -1.0f, -1.0f,  0.0f }, { 0.0f, 1.0f } },
		{ {  1.0f, -1.0f,  0.0f }, { 1.0f, 1.0f } }
	};

	GpuBufferDesc vertexBufferDesc{
		.name = "Vertex Buffer",
		.resourceType = ResourceType::VertexBuffer,
		.memoryAccess = MemoryAccess::GpuRead,
		.elementCount = vertexData.size(),
		.elementSize = sizeof(Vertex),
		.initialData = vertexData.data()
	};
	m_vertexBuffer = CreateGpuBuffer(vertexBufferDesc);

	// Setup indices
	vector<uint32_t> indexData = { 0,1,2, 2,3,0 };
	GpuBufferDesc indexBufferDesc{
		.name = "Index Buffer",
		.resourceType = ResourceType::IndexBuffer,
		.memoryAccess = MemoryAccess::GpuRead,
		.elementCount = indexData.size(),
		.elementSize = sizeof(uint32_t),
		.initialData = indexData.data()
	};
	m_indexBuffer = CreateGpuBuffer(indexBufferDesc);

	InitRootSignatures();
	
	// Create and initialize constant buffer
	m_constants.viewProjectionMatrix = m_camera.GetViewProjectionMatrix();
	m_constants.modelMatrix = Matrix4(kIdentity);
	m_constantBuffer = CreateConstantBuffer("Constant Buffer", 1, sizeof(Constants), &m_constants);

	LoadAssets();

	ColorBufferDesc scratchDesc{
		.name = "Compute Scratch Buffer",
		.width = m_texture->GetWidth(),
		.height = m_texture->GetHeight(),
		.format = Format::RGBA32_Float
	};
	m_computeScratchBuffer = CreateColorBuffer(scratchDesc);

	InitDescriptorSets();
}


void ComputeShaderApp::CreateWindowSizeDependentResources()
{
	// Create any resources that depend on window size.  May be called when the window size changes.
	if (!m_pipelinesCreated)
	{
		InitPipelines();
		m_pipelinesCreated = true;
	}

	// Update the camera since the aspect ratio might have changed
	m_camera.SetPerspectiveMatrix(
		XMConvertToRadians(60.0f),
		2.0f * GetWindowAspectRatio(),
		0.001f,
		256.0f);
}


void ComputeShaderApp::InitRootSignatures()
{
	auto graphicsDesc = RootSignatureDesc{
		.name			= "Graphics Root Signature",
		.rootParameters = {
			RootCBV(0, ShaderStage::Vertex),
			Table({ TextureSRV }, ShaderStage::Pixel)
		},
		.staticSamplers	= { StaticSampler(CommonStates::SamplerLinearClamp()) }
	};

	m_graphicsRootSignature = CreateRootSignature(graphicsDesc);

	auto computeDesc = RootSignatureDesc{
		.name			= "Compute Root Signature",
		.rootParameters = {
			Table({ TextureSRV, TextureUAV(1) }, ShaderStage::Compute),
		}
	};

	m_computeRootSignature = CreateRootSignature(computeDesc);
}


void ComputeShaderApp::InitPipelines()
{
	// Graphics pipeline
	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= sizeof(Vertex),
		.inputClassification	= InputClassification::PerVertexData
	};

	vector<VertexElementDesc> vertexElements{
		{ "POSITION", 0, Format::RGB32_Float, 0, offsetof(Vertex, position), InputClassification::PerVertexData, 0 },
		{ "TEXCOORD", 0, Format::RG32_Float, 0, offsetof(Vertex, uv), InputClassification::PerVertexData, 0 }
	};

	GraphicsPipelineDesc graphicsDesc
	{
		.name				= "Graphics Pipeline",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadOnlyReversed(),
		.rasterizerState	= CommonStates::RasterizerDefault(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "TextureVS" },
		.pixelShader		= { .shaderFile = "TexturePS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= vertexElements,
		.rootSignature		= m_graphicsRootSignature
	};

	m_graphicsPipeline = CreateGraphicsPipeline(graphicsDesc);

	// Edge detect pipeline
	ComputePipelineDesc edgeDetectDesc{
		.name			= "Edge Detect Pipeline",
		.computeShader	= { .shaderFile = "EdgeDetectCS" },
		.rootSignature	= m_computeRootSignature
	};

	m_edgeDetectPipeline = CreateComputePipeline(edgeDetectDesc);

	// Emboss pipeline
	ComputePipelineDesc embossDesc{
		.name			= "Emboss Pipeline",
		.computeShader	= {.shaderFile = "EmbossCS" },
		.rootSignature	= m_computeRootSignature
	};

	m_embossPipeline = CreateComputePipeline(embossDesc);

	// Sharpen pipeline
	ComputePipelineDesc sharpenDesc{
		.name			= "Sharpen Pipeline",
		.computeShader	= {.shaderFile = "SharpenCS" },
		.rootSignature	= m_computeRootSignature
	};

	m_sharpenPipeline = CreateComputePipeline(sharpenDesc);
}


void ComputeShaderApp::InitDescriptorSets()
{
	m_computeSrvUavDescriptorSet = m_computeRootSignature->CreateDescriptorSet(0);
	m_computeSrvUavDescriptorSet->SetSRV(0, m_texture);
	m_computeSrvUavDescriptorSet->SetUAV(1, m_computeScratchBuffer);

	m_graphicsLeftCbvDescriptorSet = m_graphicsRootSignature->CreateDescriptorSet(0);
	m_graphicsLeftCbvDescriptorSet->SetCBV(0, m_constantBuffer);

	m_graphicsLeftSrvDescriptorSet = m_graphicsRootSignature->CreateDescriptorSet(1);
	m_graphicsLeftSrvDescriptorSet->SetSRV(0, m_texture);

	m_graphicsRightCbvDescriptorSet = m_graphicsRootSignature->CreateDescriptorSet(0);
	m_graphicsRightCbvDescriptorSet->SetCBV(0, m_constantBuffer);

	m_graphicsRightSrvDescriptorSet = m_graphicsRootSignature->CreateDescriptorSet(1);
	m_graphicsRightSrvDescriptorSet->SetSRV(0, m_computeScratchBuffer);
}


void ComputeShaderApp::LoadAssets()
{
	m_texture = LoadTexture("het_kanonschot_rgba8.ktx");
}