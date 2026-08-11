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

#include "VariableRateShadingApp.h"

#include "BinaryReader.h"
#include "FileSystem.h"
#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"
#include "Graphics\Device.h"

#include "SquidRoom.h"

using namespace Luna;
using namespace Math;
using namespace std;


VariableRateShadingApp::VariableRateShadingApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{
}


int VariableRateShadingApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void VariableRateShadingApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void VariableRateShadingApp::Startup()
{
	// Application initialization, after device creation
}


void VariableRateShadingApp::Shutdown()
{
	// Application cleanup on shutdown
}


void VariableRateShadingApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffers();
}


void VariableRateShadingApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(GetDepthBuffer(), ResourceState::DepthWrite);
	context.ClearColor(GetColorBuffer());
	context.ClearDepthAndStencil(GetDepthBuffer());

	context.BeginRendering(GetColorBuffer(), GetDepthBuffer());

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());
	context.SetRootSignature(m_sceneRootSignature);
	context.SetGraphicsPipeline(m_sceneGraphicsPipeline);

	context.SetIndexBuffer(m_indexBuffer);
	context.SetVertexBuffer(0, m_vertexBuffer);

	context.SetRootCBV(0, m_sceneConstantBuffer);

	for (int i = 0; i < _countof(SampleAssets::s_draws); ++i)
	{
		const SampleAssets::DrawParameters& drawArgs = SampleAssets::s_draws[i];
		context.SetSRV(1, 0, m_textures[drawArgs.diffuseTextureIndex]);
		context.SetSRV(1, 1, m_textures[drawArgs.normalTextureIndex]);

		context.DrawIndexedInstanced(drawArgs.indexCount, 1, drawArgs.indexStart, drawArgs.vertexBase, 0);
	}

	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void VariableRateShadingApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		512.0f);
	Vector3 cameraPosition{ 0.0f, 17.1954231f, -28.555980f };
	m_camera.SetPosition(cameraPosition);

	m_sceneConstantBuffer = CreateConstantBuffer("Scene Constant Buffer", 1, sizeof(SceneConstants));
	m_shadowConstantBuffer = CreateConstantBuffer("Shadow Constant Buffer", 1, sizeof(SceneConstants));

	LoadAssets();
	InitRootSignature();

	m_controller.RefreshFromCamera();
	m_controller.SetCameraMode(CameraMode::ArcBall);
	Vector3 target{ 0.0f, 8.0f, 0.0f };
	m_controller.SetOrbitTarget(target, Length(cameraPosition - target), 0.25f);
	m_controller.SlowMovement(true);
	m_controller.SlowRotation(false);
	m_controller.SetSpeedScale(0.25f);
}


void VariableRateShadingApp::CreateWindowSizeDependentResources()
{
	// Create any resources that depend on window size.  May be called when the window size changes.
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


void VariableRateShadingApp::InitRootSignature()
{
	RootSignatureDesc desc{
		.name				= "Scene Root Signature",
		.rootParameters		= {
			RootCBV(0, ShaderStage::Vertex | ShaderStage::Pixel),
			Table({ TextureSRV(0, 2) }, ShaderStage::Pixel)
		},
		.staticSamplers		= { 
			StaticSampler(CommonStates::SamplerLinearClamp(), ShaderStage::Pixel), 
			StaticSampler(CommonStates::SamplerLinearWrap(), ShaderStage::Pixel) 
		}
	};
	m_sceneRootSignature = CreateRootSignature(desc);
}


void VariableRateShadingApp::InitPipeline()
{
	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= sizeof(Vertex),
		.inputClassification	= InputClassification::PerVertexData
	};

	GraphicsPipelineDesc desc{
		.name				= "Graphics Pipeline",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState	= CommonStates::RasterizerDefault(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "ShadowsAndSceneVS" },
		.pixelShader		= { .shaderFile = "ShadowsAndScenePS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= SampleAssets::s_standardVertexElements,
		.rootSignature		= m_sceneRootSignature
	};

	m_sceneGraphicsPipeline = CreateGraphicsPipeline(desc);
}


void VariableRateShadingApp::LoadAssets()
{
	using namespace SampleAssets;

	string filepath = m_fileSystem->GetFullPath(s_dataFileName);

	size_t dataSize = 0;
	unique_ptr<std::byte[]> data;
	BinaryReader::ReadEntireFile(filepath, data, &dataSize);

	// Create vertex and index buffers
	size_t numVertices = s_vertexDataSize / s_standardVertexStride;
	m_vertexBuffer = CreateVertexBuffer("Vertex Buffer", numVertices, s_standardVertexStride, data.get() + s_vertexDataOffset);

	// Compute scene bounding box
	Vertex* currentVertex = (Vertex*)(data.get() + s_vertexDataOffset);
	constexpr float maxF = numeric_limits<float>::max();
	Vector3 minExtents{ maxF, maxF, maxF };
	Vector3 maxExtents{ -maxF, -maxF, -maxF };
	for (uint32_t i = 0; i < numVertices; ++i)
	{
		Vector3 pos{ currentVertex->pos[0], currentVertex->pos[1], currentVertex->pos[2] };
		minExtents = Math::Min(minExtents, pos);
		maxExtents = Math::Max(maxExtents, pos);
		++currentVertex;
	}
	m_sceneBoundingBox = Math::BoundingBoxFromMinMax(minExtents, maxExtents);

	// Create index buffer
	GpuBufferDesc indexBufferDesc{
		.name			= "Index Buffer",
		.resourceType	= ResourceType::IndexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= s_indexDataSize / sizeof(uint32_t),
		.elementSize	= sizeof(uint32_t),
		.initialData	= data.get() + s_indexDataOffset
	};
	m_indexBuffer = CreateGpuBuffer(indexBufferDesc);

	// Create textures
	uint32_t i = 0;
	auto device = GetDevice();
	for (const auto& textureData : s_textures)
	{
		string texName = format("SquidRoom_Texture_{}.dds", i++);

		TextureDesc desc{
			.name		= texName,
			.width		= textureData.width,
			.height		= textureData.height,
			.depth		= 1,
			.numMips	= 1,
			.format		= textureData.format,
			.dataSize	= textureData.data->size,
			.data		= data.get() + textureData.data->offset
		};

		TexturePtr texture = device->CreateTexture2D(desc);

		m_textures.push_back(texture);
	}
}


void VariableRateShadingApp::UpdateConstantBuffers()
{
	// Scale down the world a bit.
	const float worldScale = GetWorldScale();
	Math::Matrix4 worldScaleMatrix = Math::Matrix4::MakeScale(worldScale);
	m_sceneConstants.modelMatrix = worldScaleMatrix;
	m_shadowConstants.modelMatrix = worldScaleMatrix;

	auto width = GetWindowWidth();
	auto height = GetWindowHeight();

	m_sceneConstants.viewport = Math::Vector4{ (float)width, (float)height, 0.0f, 0.0f };

	// The scene pass is drawn from the main camera
	m_sceneConstants.viewMatrix = m_camera.GetViewMatrix();
	m_sceneConstants.projectionMatrix = m_camera.GetProjectionMatrix();

	// The shadow pass is drawn from the first light
	m_shadowConstants.viewMatrix = m_lightCameras[0].GetViewMatrix();
	m_shadowConstants.projectionMatrix = m_lightCameras[0].GetProjectionMatrix();

	for (int i = 0; i < m_numLights; ++i)
	{
		memcpy(&m_sceneConstants.lights[i], &m_lightState[i], sizeof(LightState));
		memcpy(&m_shadowConstants.lights[i], &m_lightState[i], sizeof(LightState));
	}

	// The shadow pass won't sample the shadow map, but rather write to it.
	m_shadowConstants.sampleShadowMap = FALSE;

	// The scene pass samples the shadow map.
	m_sceneConstants.sampleShadowMap = TRUE;

	m_shadowConstants.ambientColor = m_sceneConstants.ambientColor = { 0.1f, 0.2f, 0.3f, 1.0f };

	// Update the constant buffers
	m_sceneConstantBuffer->Update(sizeof(SceneConstants), &m_sceneConstants);
	m_shadowConstantBuffer->Update(sizeof(SceneConstants), &m_shadowConstants);
}