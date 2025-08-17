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

#include "DescriptorIndexingApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"

using namespace Luna;
using namespace Math;
using namespace std;


DescriptorIndexingApp::DescriptorIndexingApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{
}


int DescriptorIndexingApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void DescriptorIndexingApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void DescriptorIndexingApp::Startup()
{
	// Application initialization, after device creation
}


void DescriptorIndexingApp::Shutdown()
{
	// Application cleanup on shutdown
}


void DescriptorIndexingApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffer();
}


void DescriptorIndexingApp::Render()
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

	context.SetDescriptors(0, m_vsDescriptorSet);
	context.SetDescriptors(1, m_psDescriptorSet);

	context.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
	context.SetVertexBuffer(0, m_vertexBuffer);
	context.SetIndexBuffer(m_indexBuffer);

	context.DrawIndexed((uint32_t)m_indexBuffer->GetElementCount());

	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void DescriptorIndexingApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		512.0f);
	Vector3 cameraPosition{ 5.0f, 5.0f, 5.0f };
	m_camera.SetPosition(cameraPosition);

	GenerateTextures();
	GenerateCubes();

	// Create and initialize constant buffer
	m_constantBuffer = CreateConstantBuffer("Constant Buffer", 1, sizeof(Constants));
	UpdateConstantBuffer();

	InitRootSignature();
	InitDescriptorSets();

	m_controller.RefreshFromCamera();
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.SetOrbitTarget(Vector3(kZero), Length(cameraPosition), 0.25f);
	m_controller.SlowMovement(true);
	m_controller.SlowRotation(false);
	m_controller.SetSpeedScale(0.25f);
}


void DescriptorIndexingApp::CreateWindowSizeDependentResources()
{
	if (!m_pipelineCreated)
	{
		InitPipeline();
		m_pipelineCreated = true;
	}

	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		512.0f);
}


void DescriptorIndexingApp::InitRootSignature()
{
	RootSignatureDesc desc{
		.name = "Root Signature",
		.rootParameters = {
			Table({ ConstantBuffer }, ShaderStage::Vertex),
			Table({ BindlessTextureSRV(0, m_numTextures) }, ShaderStage::Pixel)
		},
		.staticSamplers = { StaticSampler(CommonStates::SamplerPointClamp()) }
	};
	m_rootSignature = CreateRootSignature(desc);
}


void DescriptorIndexingApp::InitPipeline()
{
	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= sizeof(Vertex),
		.inputClassification	= InputClassification::PerVertexData
	};

	vector<VertexElementDesc> vertexElements{
		{ "POSITION", 0, Format::RGB32_Float, 0, offsetof(Vertex, pos), InputClassification::PerVertexData, 0 },
		{ "TEXCOORD", 0, Format::RG32_Float, 0, offsetof(Vertex, uv), InputClassification::PerVertexData, 0 },
		{ "TEXTUREINDEX", 0, Format::R32_SInt, 0, offsetof(Vertex, textureIndex), InputClassification::PerVertexData, 0 }
	};

	GraphicsPipelineDesc pipelineDesc{
		.name				= "Graphics Pipeline",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState	= CommonStates::RasterizerTwoSided(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "DescriptorIndexingVS" },
		.pixelShader		= { .shaderFile = "DescriptorIndexingPS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= vertexElements,
		.rootSignature		= m_rootSignature
	};
	m_graphicsPipeline = CreateGraphicsPipeline(pipelineDesc);
}


void DescriptorIndexingApp::InitDescriptorSets()
{
	m_vsDescriptorSet = m_rootSignature->CreateDescriptorSet(0);
	m_vsDescriptorSet->SetCBV(0, m_constantBuffer->GetCbvDescriptor());

	std::vector<const IDescriptor*> textureDescriptors{ m_numTextures };
	for (size_t i = 0; i < textureDescriptors.size(); ++i)
	{
		textureDescriptors[i] = m_textures[i]->GetDescriptor();
	}
	
	m_psDescriptorSet = m_rootSignature->CreateDescriptorSet(1);
	m_psDescriptorSet->SetBindlessSRVs(0, textureDescriptors);
}


void DescriptorIndexingApp::GenerateTextures()
{
	m_textures.clear();
	m_textures.resize(m_numTextures);
	
	const uint32_t width = 4;
	const uint32_t channels = 4;
	array<uint8_t, width * width * channels> data;

	RandomNumberGenerator rng;

	for (uint32_t i = 0; i < m_numTextures; ++i)
	{
		for (uint32_t j = 0; j < width * width; ++j)
		{
			data[j * 4 + 0] = (uint8_t)rng.NextInt(50, 255);
			data[j * 4 + 1] = (uint8_t)rng.NextInt(50, 255);
			data[j * 4 + 2] = (uint8_t)rng.NextInt(50, 255);
			data[j * 4 + 3] = 255;
		}

		TextureDesc desc{
			.name		= format("Texture {}", i),
			.width		= width,
			.height		= width,
			.format		= Format::RGBA8_UNorm,
			.dataSize	= width * width * channels,
			.data		= (std::byte*)data.data()
		};
		auto texture = CreateTexture2D(desc);
		m_textures[i] = texture;
	}
}


void DescriptorIndexingApp::GenerateCubes()
{
	vector<Vertex> vertices;
	vector<uint16_t> indices;
	
	RandomNumberGenerator rng;
	array<int32_t, 6> textureIndices;

	// Create cubes
	for (uint32_t i = 0; i < m_numCubes; ++i)
	{
		// Cube indices
		const std::vector<uint16_t> cubeIndices = 
		{
			0,1,2,0,2,3,
			4,5,6,4,6,7,
			8,9,10,8,10,11,
			12,13,14,12,14,15,
			16,17,18,16,18,19,
			20,21,22,20,22,23
		};

		for (auto& index : cubeIndices) 
		{
			indices.push_back(index + static_cast<uint16_t>(vertices.size()));
		}

		// Random texture indices
		for (uint32_t j = 0; j < 6; ++j)
		{
			textureIndices[j] = rng.NextInt(0, m_numTextures-1);
		}

		// Cube vertices
		float pos = 2.5f * i - (m_numCubes * 2.5f / 2.0f) + 1.25f;
		const vector<Vertex> cube = 
		{
			{ { -1.0f + pos, -1.0f,  1.0f }, { 0.0f, 0.0f }, textureIndices[0] },
			{ {  1.0f + pos, -1.0f,  1.0f }, { 1.0f, 0.0f }, textureIndices[0] },
			{ {  1.0f + pos,  1.0f,  1.0f }, { 1.0f, 1.0f }, textureIndices[0] },
			{ { -1.0f + pos,  1.0f,  1.0f }, { 0.0f, 1.0f }, textureIndices[0] },

			{ {  1.0f + pos,  1.0f,  1.0f }, { 0.0f, 0.0f }, textureIndices[1] },
			{ {  1.0f + pos,  1.0f, -1.0f }, { 1.0f, 0.0f }, textureIndices[1] },
			{ {  1.0f + pos, -1.0f, -1.0f }, { 1.0f, 1.0f }, textureIndices[1] },
			{ {  1.0f + pos, -1.0f,  1.0f }, { 0.0f, 1.0f }, textureIndices[1] },

			{ { -1.0f + pos, -1.0f, -1.0f }, { 0.0f, 0.0f }, textureIndices[2] },
			{ {  1.0f + pos, -1.0f, -1.0f }, { 1.0f, 0.0f }, textureIndices[2] },
			{ {  1.0f + pos,  1.0f, -1.0f }, { 1.0f, 1.0f }, textureIndices[2] },
			{ { -1.0f + pos,  1.0f, -1.0f }, { 0.0f, 1.0f }, textureIndices[2] },

			{ { -1.0f + pos, -1.0f, -1.0f }, { 0.0f, 0.0f }, textureIndices[3] },
			{ { -1.0f + pos, -1.0f,  1.0f }, { 1.0f, 0.0f }, textureIndices[3] },
			{ { -1.0f + pos,  1.0f,  1.0f }, { 1.0f, 1.0f }, textureIndices[3] },
			{ { -1.0f + pos,  1.0f, -1.0f }, { 0.0f, 1.0f }, textureIndices[3] },

			{ {  1.0f + pos,  1.0f,  1.0f }, { 0.0f, 0.0f }, textureIndices[4] },
			{ { -1.0f + pos,  1.0f,  1.0f }, { 1.0f, 0.0f }, textureIndices[4] },
			{ { -1.0f + pos,  1.0f, -1.0f }, { 1.0f, 1.0f }, textureIndices[4] },
			{ {  1.0f + pos,  1.0f, -1.0f }, { 0.0f, 1.0f }, textureIndices[4] },

			{ { -1.0f + pos, -1.0f, -1.0f }, { 0.0f, 0.0f }, textureIndices[5] },
			{ {  1.0f + pos, -1.0f, -1.0f }, { 1.0f, 0.0f }, textureIndices[5] },
			{ {  1.0f + pos, -1.0f,  1.0f }, { 1.0f, 1.0f }, textureIndices[5] },
			{ { -1.0f + pos, -1.0f,  1.0f }, { 0.0f, 1.0f }, textureIndices[5] },
		};

		for (auto& vertex : cube) 
		{
			vertices.push_back(vertex);
		}
	}

	m_vertexBuffer = CreateVertexBuffer<Vertex>("Vertex Buffer", vertices);
	m_indexBuffer = CreateIndexBuffer("Index Buffer", indices);
}


void DescriptorIndexingApp::UpdateConstantBuffer()
{
	m_constants.projectionMatrix = m_camera.GetProjectionMatrix();
	m_constants.viewMatrix = m_camera.GetViewMatrix();
	m_constantBuffer->Update(sizeof(Constants), &m_constants);
}