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

#include "InstancingApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"

using namespace Luna;
using namespace Math;
using namespace std;


InstancingApp::InstancingApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{}


int InstancingApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void InstancingApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void InstancingApp::Startup()
{
	// Application initialization, after device creation
}


void InstancingApp::Shutdown()
{
	// Application cleanup on shutdown
}


void InstancingApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffer();
}


void InstancingApp::UpdateUI()
{
	if (m_uiOverlay->Header("Statistics"))
	{
		m_uiOverlay->Text("Instances: %d", m_numInstances);
	}
}


void InstancingApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(GetDepthBuffer(), ResourceState::DepthWrite);
	context.ClearColor(GetColorBuffer());
	context.ClearDepthAndStencil(GetDepthBuffer());

	context.BeginRendering(GetColorBuffer(), GetDepthBuffer());

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());

	// Starfield
	{
		context.SetRootSignature(m_starfieldRootSignature);
		context.SetGraphicsPipeline(m_starfieldPipeline);

		context.Draw(4);
	}

	// Planet
	{
		context.SetRootSignature(m_modelRootSignature);
		context.SetGraphicsPipeline(m_planetPipeline);

		context.SetResources(m_planetResources);

		m_planetModel->Render(context);
	}

	// Rocks
	{
		context.SetGraphicsPipeline(m_rockPipeline);

		context.SetResources(m_rockResources);

		{
			for(const auto mesh : m_rockModel->meshes)
			{
				context.SetIndexBuffer(mesh->indexBuffer);
				context.SetVertexBuffer(0, mesh->vertexBuffer);
				context.SetVertexBuffer(1, m_instanceBuffer);

				for (const auto meshPart : mesh->meshParts)
				{
					context.DrawIndexedInstanced(meshPart.indexCount, m_numInstances, meshPart.indexBase, meshPart.vertexBase, 0);
				}
			}
		}
	}

	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void InstancingApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		256.0f);
	AffineTransform transform(Quaternion(DirectX::XMConvertToRadians(-17.2f), DirectX::XMConvertToRadians(-4.7f), 0.0f), Vector3(5.5f, -1.85f, -18.5f));
	m_camera.SetTransform(transform);

	m_controller.SetSpeedScale(0.01f);
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.RefreshFromCamera();
	m_controller.SetOrbitTarget(Vector3(0.0f, 0.0f, 0.0f), Length(m_camera.GetPosition()), 0.25f);

	InitRootSignatures();
	
	// Create constant buffer
	m_planetConstantBuffer = CreateConstantBuffer("Planet Constant Buffer", 1, sizeof(VSConstants));

	// Load assets first, since instancing data needs to know how many array slices are in
	// the rock texture
	LoadAssets();
	InitInstanceBuffer();

	InitResourceSets();
}


void InstancingApp::CreateWindowSizeDependentResources()
{
	if (!m_pipelinesCreated)
	{
		InitPipelines();
		m_pipelinesCreated = true;
	}

	// Update the camera, since the aspect ratio might have changed
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		256.0f);
}


void InstancingApp::InitRootSignatures()
{
	RootSignatureDesc starfieldRootSignatureDesc{
		.name	= "Starfield Root Signature",
		.flags	= RootSignatureFlags::AllowInputAssemblerInputLayout |
					RootSignatureFlags::DenyPixelShaderRootAccess
	};
	m_starfieldRootSignature = CreateRootSignature(starfieldRootSignatureDesc);

	RootSignatureDesc modelRootSignatureDesc{
		.name				= "Model Root Signature",
		.flags				= RootSignatureFlags::AllowInputAssemblerInputLayout,
		.rootParameters		= {	
			RootCBV(0, ShaderStage::Vertex),	
			Table({ TextureSRV }, ShaderStage::Pixel),
			Table({ Sampler }, ShaderStage::Pixel)
		}
	};
	m_modelRootSignature = CreateRootSignature(modelRootSignatureDesc);
}


void InstancingApp::InitPipelines()
{
	// Starfield
	GraphicsPipelineDesc starfieldPipelineDesc{
		.name				= "Starfield Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateDisabled(),
		.rasterizerState	= CommonStates::RasterizerDefault(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "StarfieldVS" },
		.pixelShader		= {.shaderFile = "StarfieldPS" },
		.rootSignature		= m_starfieldRootSignature
	};
	m_starfieldPipeline = CreateGraphicsPipeline(starfieldPipelineDesc);

	// Instanced rock
	vector<VertexStreamDesc> instancedVertexStreams = {
		{ 0, 12 * sizeof(float), InputClassification::PerVertexData },
		{ 1, 8 * sizeof(float), InputClassification::PerInstanceData }
	};

	vector<VertexElementDesc> instancedVertexElements =
	{
		{ "POSITION", 0, Format::RGB32_Float, 0, 0, InputClassification::PerVertexData, 0 },
		{ "NORMAL", 0, Format::RGB32_Float, 0, 3 * sizeof(float), InputClassification::PerVertexData, 0 },
		{ "COLOR", 0, Format::RGBA32_Float, 0, 6 * sizeof(float), InputClassification::PerVertexData, 0 },
		{ "TEXCOORD", 0, Format::RG32_Float, 0, 10 * sizeof(float), InputClassification::PerVertexData, 0 },

		{ "INSTANCED_POSITION", 0, Format::RGB32_Float, 1, 0, InputClassification::PerInstanceData, 1 },
		{ "INSTANCED_ROTATION", 0, Format::RGB32_Float, 1, 3 * sizeof(float), InputClassification::PerInstanceData, 1 },
		{ "INSTANCED_SCALE", 0, Format::R32_Float, 1, 6 * sizeof(float), InputClassification::PerInstanceData, 1 },
		{ "INSTANCED_TEX_INDEX", 0, Format::R32_UInt, 1, 7 * sizeof(float), InputClassification::PerInstanceData, 1 }
	};

	GraphicsPipelineDesc rockPipelineDesc{
		.name				= "Rock Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState	= CommonStates::RasterizerDefault(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= {.shaderFile = "InstancingVS" },
		.pixelShader		= {.shaderFile = "InstancingPS" },
		.vertexStreams		= instancedVertexStreams,
		.vertexElements		= instancedVertexElements,
		.rootSignature		= m_modelRootSignature
	};
	m_rockPipeline = CreateGraphicsPipeline(rockPipelineDesc);

	// Planet
	auto vertexLayout = VertexLayout<VertexComponent::PositionNormalColorTexcoord>();
	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= vertexLayout.GetSizeInBytes(),
		.inputClassification	= InputClassification::PerVertexData
	};

	auto layout = VertexLayout<VertexComponent::PositionNormalColor>();

	GraphicsPipelineDesc planetPipelineDesc{
		.name				= "Planet Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState	= CommonStates::RasterizerDefault(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "PlanetVS" },
		.pixelShader		= { .shaderFile = "PlanetPS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= vertexLayout.GetElements(),
		.rootSignature		= m_modelRootSignature
	};
	m_planetPipeline = CreateGraphicsPipeline(planetPipelineDesc);
}


void InstancingApp::InitInstanceBuffer()
{
	struct InstanceData
	{
		float pos[3];
		float rot[3];
		float scale;
		uint32_t index;
	};

	const int32_t numLayers = (int32_t)m_rockTexture->GetArraySize();

	vector<InstanceData> instanceData;
	instanceData.resize(m_numInstances);

	Math::RandomNumberGenerator rng;

	for (uint32_t i = 0; i < m_numInstances / 2; ++i)
	{
		float ring0[] = { 7.0f, 11.0f };
		float ring1[] = { 14.0f, 18.0f };

		float rho = sqrtf((powf(ring0[1], 2.0f) - pow(ring0[0], 2.0f)) * rng.NextFloat() + powf(ring0[0], 2.0f));
		float theta = DirectX::XM_2PI * rng.NextFloat();

		instanceData[i].pos[0] = rho * cosf(theta);
		instanceData[i].pos[1] = rng.NextFloat(-0.25f, 0.25f);
		instanceData[i].pos[2] = rho * sinf(theta);
		instanceData[i].rot[0] = rng.NextFloat(0, DirectX::XM_PI);
		instanceData[i].rot[1] = rng.NextFloat(0, DirectX::XM_PI);
		instanceData[i].rot[2] = rng.NextFloat(0, DirectX::XM_PI);
		instanceData[i].scale = 0.75f * (1.5f + rng.NextFloat() - rng.NextFloat());
		instanceData[i].index = rng.NextInt(0, numLayers - 1);

		rho = sqrtf((powf(ring1[1], 2.0f) - pow(ring1[0], 2.0f)) * rng.NextFloat() + powf(ring1[0], 2.0f));
		theta = DirectX::XM_2PI * rng.NextFloat();

		uint32_t i2 = i + m_numInstances / 2;
		instanceData[i2].pos[0] = rho * cosf(theta);
		instanceData[i2].pos[1] = rng.NextFloat(-0.25f, 0.25f);
		instanceData[i2].pos[2] = rho * sinf(theta);
		instanceData[i2].rot[0] = rng.NextFloat(0, DirectX::XM_PI);
		instanceData[i2].rot[1] = rng.NextFloat(0, DirectX::XM_PI);
		instanceData[i2].rot[2] = rng.NextFloat(0, DirectX::XM_PI);
		instanceData[i2].scale = 0.75f * (1.5f + rng.NextFloat() - rng.NextFloat());
		instanceData[i2].index = rng.NextInt(0, numLayers - 1);
	}

	GpuBufferDesc desc{
		.name			= "Instance Buffer",
		.resourceType	= ResourceType::VertexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= m_numInstances,
		.elementSize	= sizeof(InstanceData),
		.initialData	= instanceData.data()
	};
	m_instanceBuffer = CreateGpuBuffer(desc);
}


void InstancingApp::InitResourceSets()
{
	m_rockResources.Initialize(m_modelRootSignature);
	m_rockResources.SetCBV(0, 0, m_planetConstantBuffer);
	m_rockResources.SetSRV(1, 0, m_rockTexture);
	m_rockResources.SetSampler(2, 0, m_sampler);

	m_planetResources.Initialize(m_modelRootSignature);
	m_planetResources.SetCBV(0, 0, m_planetConstantBuffer);
	m_planetResources.SetSRV(1, 0, m_planetTexture);
	m_planetResources.SetSampler(2, 0, m_sampler);
}


void InstancingApp::UpdateConstantBuffer()
{
	using namespace Math;

	m_planetConstants.projectionMatrix = m_camera.GetProjectionMatrix();

	Quaternion rotX{ Vector3(kXUnitVector), XMConvertToRadians(17.2f) };
	Quaternion rotY{ Vector3(kYUnitVector), XMConvertToRadians(4.7f) };
	Quaternion rotZ{ Vector3(kZUnitVector), XMConvertToRadians(0.0f) };

	Quaternion rotTotal = rotX * rotY * rotZ;

	Matrix4 viewMatrix{ AffineTransform(rotTotal, Vector3(5.5f, 1.85f, 0.0f) + Vector3(0.0f, 0.0f, m_zoom)) };

	m_planetConstants.modelViewMatrix = viewMatrix;
	m_planetConstants.localSpeed += (float)m_timer.GetElapsedSeconds() * 0.35f;
	m_planetConstants.globalSpeed += (float)m_timer.GetElapsedSeconds() * 0.01f;

	m_planetConstantBuffer->Update(sizeof(m_planetConstants), &m_planetConstants);
}


void InstancingApp::LoadAssets()
{
	m_rockTexture = LoadTexture("texturearray_rocks_rgba.ktx", Format::Unknown, true);
	m_planetTexture = LoadTexture("lavaplanet_rgba.ktx", Format::Unknown, true);
	m_sampler = CreateSampler(CommonStates::SamplerLinearWrap());

	auto layout = VertexLayout<VertexComponent::PositionNormalColorTexcoord>();

	m_rockModel = LoadModel("rock01.gltf", layout, 1.0f);
	m_planetModel = LoadModel("sphere.gltf", layout, 1.0f);
}