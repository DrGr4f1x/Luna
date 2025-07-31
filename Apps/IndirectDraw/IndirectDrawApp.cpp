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

#include "IndirectDrawApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"

using namespace Luna;
using namespace Math;
using namespace std;


IndirectDrawApp::IndirectDrawApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{}


int IndirectDrawApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void IndirectDrawApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void IndirectDrawApp::Startup()
{
	// Application initialization, after device creation
}


void IndirectDrawApp::Shutdown()
{
	// Application cleanup on shutdown
}


void IndirectDrawApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffer();
}


void IndirectDrawApp::UpdateUI()
{

}


void IndirectDrawApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(GetDepthBuffer(), ResourceState::DepthWrite);
	context.ClearColor(GetColorBuffer());
	context.ClearDepthAndStencil(GetDepthBuffer());

	context.BeginRendering(GetColorBuffer(), GetDepthBuffer());

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());

	// Sky sphere
	{
		context.SetRootSignature(m_skySphereRootSignature);
		context.SetGraphicsPipeline(m_skySpherePipeline);

		context.SetDescriptors(0, m_cbvDescriptorSet);

		m_skySphereModel->Render(context);
	}

	// Ground plane
	{
		context.SetRootSignature(m_groundRootSignature);
		context.SetGraphicsPipeline(m_groundPipeline);

		context.SetDescriptors(0, m_cbvDescriptorSet);
		context.SetDescriptors(1, m_groundSrvDescriptorSet);

		m_groundModel->Render(context);
	}

	// Plants
	{
		context.SetRootSignature(m_plantsRootSignature);
		context.SetGraphicsPipeline(m_plantsPipeline);
		
		context.SetDescriptors(0, m_cbvDescriptorSet);
		context.SetDescriptors(1, m_plantsSrvDescriptorSet);

		// TODO: Support multi-draw
		if (false)
		{

		}
		else
		{
			context.SetVertexBuffer(1, m_instanceBuffer);

			const auto numIndirectCommands = m_indirectArgsBuffer->GetElementCount();
			for (auto i = 0; i < numIndirectCommands; ++i)
			{
				const auto& mesh = m_plantsModel->meshes[i];
				context.SetIndexBuffer(mesh->indexBuffer);
				context.SetVertexBuffer(0, mesh->vertexBuffer);
				context.DrawIndexedIndirect(m_indirectArgsBuffer, i * sizeof(DrawIndexedIndirectArgs));
			}
		}
	}

	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void IndirectDrawApp::CreateDeviceDependentResources()
{
	using namespace DirectX;

	// Create any resources that depend on the device, but not the window size
	m_camera.SetPerspectiveMatrix(
		XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		512.0f);
	m_camera.SetPosition(Vector3{ 7.0f, 4.0f, 7.0f });
	
	InitRootSignatures();
	
	m_vsConstantBuffer = CreateConstantBuffer("VS Constant Buffer", 1, sizeof(VSConstants));

	LoadAssets();
	InitIndirectArgs();
	InitInstanceData();
	InitDescriptorSets();

	m_controller.RefreshFromCamera();
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.SetOrbitTarget(Vector3{ kZero }, 10.0f, 0.25f);
	m_controller.SlowMovement(true);
	m_controller.SlowRotation(false);
	m_controller.SetSpeedScale(0.25f);
}


void IndirectDrawApp::CreateWindowSizeDependentResources()
{
	// Create any resources that depend on window size.  May be called when the window size changes.
	if (!m_pipelinesCreated)
	{
		InitPipelines();
		m_pipelinesCreated = true;
	}

	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		512.0f);
}


void IndirectDrawApp::InitRootSignatures()
{
	// Sky sphere
	RootSignatureDesc skySphereRootSignatureDesc{
		.name				= "Sky Sphere Root Signature",
		.flags				= RootSignatureFlags::AllowInputAssemblerInputLayout | RootSignatureFlags::DenyPixelShaderRootAccess,
		.rootParameters		= { RootCBV(0, ShaderStage::Vertex) }
	};
	m_skySphereRootSignature = CreateRootSignature(skySphereRootSignatureDesc);

	// Ground plane
	RootSignatureDesc groundRootSignatureDesc{
		.name				= "Ground Root Signature",
		.flags				= RootSignatureFlags::AllowInputAssemblerInputLayout,
		.rootParameters		= {
			RootCBV(0, ShaderStage::Vertex),
			Table({ TextureSRV }, ShaderStage::Pixel)
		},
		.staticSamplers		= { StaticSampler(CommonStates::SamplerLinearWrap()) }
	};
	m_groundRootSignature = CreateRootSignature(groundRootSignatureDesc);

	// Plants
	RootSignatureDesc plantsRootSignatureDesc{
		.name				= "Plants Root Signature",
		.flags				= RootSignatureFlags::AllowInputAssemblerInputLayout,
		.rootParameters		= {
			RootCBV(0, ShaderStage::Vertex),
			Table({ TextureSRV }, ShaderStage::Pixel)
		},
		.staticSamplers = { StaticSampler(CommonStates::SamplerLinearClamp()) }
	};
	m_plantsRootSignature = CreateRootSignature(plantsRootSignatureDesc);
}


void IndirectDrawApp::InitPipelines()
{
	// Sky sphere pipeline
	{
		auto vertexLayout = VertexLayout<VertexComponent::PositionTexcoord>();

		VertexStreamDesc vertexStreamDesc{
			.inputSlot				= 0,
			.stride					= vertexLayout.GetSizeInBytes(),
			.inputClassification	= InputClassification::PerVertexData
		};

		GraphicsPipelineDesc skySpherePipelineDesc{
			.name				= "Sky Sphere Graphics PSO",
			.blendState			= CommonStates::BlendDisable(),
			.depthStencilState	= CommonStates::DepthStateDisabled(),
			.rasterizerState	= CommonStates::RasterizerDefaultCW(),
			.rtvFormats			= { GetColorFormat() },
			.dsvFormat			= GetDepthFormat(),
			.topology			= PrimitiveTopology::TriangleList,
			.vertexShader		= { .shaderFile = "SkySphereVS" },
			.pixelShader		= { .shaderFile = "SkySpherePS" },
			.vertexStreams		= { vertexStreamDesc },
			.vertexElements		= vertexLayout.GetElements(),
			.rootSignature		= m_skySphereRootSignature
		};
		m_skySpherePipeline = CreateGraphicsPipeline(skySpherePipelineDesc);
	}

	// Ground pipeline
	{
		auto vertexLayout = VertexLayout<VertexComponent::PositionNormalColorTexcoord>();

		VertexStreamDesc vertexStreamDesc{
			.inputSlot				= 0,
			.stride					= vertexLayout.GetSizeInBytes(),
			.inputClassification	= InputClassification::PerVertexData
		};

		GraphicsPipelineDesc groundPipelineDesc{
			.name				= "Ground Graphics PSO",
			.blendState			= CommonStates::BlendDisable(),
			.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
			.rasterizerState	= CommonStates::RasterizerDefault(),
			.rtvFormats			= { GetColorFormat() },
			.dsvFormat			= GetDepthFormat(),
			.topology			= PrimitiveTopology::TriangleList,
			.vertexShader		= { .shaderFile = "GroundVS" },
			.pixelShader		= { .shaderFile = "GroundPS" },
			.vertexStreams		= { vertexStreamDesc },
			.vertexElements		= vertexLayout.GetElements(),
			.rootSignature		= m_groundRootSignature
		};
		m_groundPipeline = CreateGraphicsPipeline(groundPipelineDesc);
	}

	// Plants pipeline
	{
		vector<VertexStreamDesc> instancedVertexStreams{
			{ 0, 12 * sizeof(float), InputClassification::PerVertexData },
			{ 1, 8 * sizeof(float), InputClassification::PerInstanceData }
		};

		vector<VertexElementDesc> instancedVertexElements =
		{
			{ "POSITION", 0, Format::RGB32_Float, 0, 0, InputClassification::PerVertexData, 0},
			{ "NORMAL", 0, Format::RGB32_Float, 0, 3 * sizeof(float), InputClassification::PerVertexData, 0 },
			{ "COLOR", 0, Format::RGBA32_Float, 0, 6 * sizeof(float), InputClassification::PerVertexData, 0 },
			{ "TEXCOORD", 0, Format::RG32_Float, 0, 10 * sizeof(float), InputClassification::PerVertexData, 0 },

			{ "INSTANCED_POSITION", 0, Format::RGB32_Float, 1, offsetof(InstanceData, position), InputClassification::PerInstanceData, 1},
			{ "INSTANCED_ROTATION", 0, Format::RGB32_Float, 1, offsetof(InstanceData, rotation), InputClassification::PerInstanceData, 1},
			{ "INSTANCED_SCALE", 0, Format::R32_Float, 1, offsetof(InstanceData, scale), InputClassification::PerInstanceData, 1},
			{ "INSTANCED_TEX_INDEX", 0, Format::R32_SInt, 1, offsetof(InstanceData, texIndex), InputClassification::PerInstanceData, 1}
		};

		GraphicsPipelineDesc plantsPipelineDesc{
			.name				= "Plants Graphics PSO",
			.blendState			= CommonStates::BlendDisable(),
			.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
			.rasterizerState	= CommonStates::RasterizerDefault(),
			.rtvFormats			= { GetColorFormat() },
			.dsvFormat			= GetDepthFormat(),
			.topology			= PrimitiveTopology::TriangleList,
			.vertexShader		= { .shaderFile = "IndirectDrawVS" },
			.pixelShader		= { .shaderFile = "IndirectDrawPS" },
			.vertexStreams		= instancedVertexStreams,
			.vertexElements		= instancedVertexElements,
			.rootSignature		= m_plantsRootSignature
		};
		m_plantsPipeline = CreateGraphicsPipeline(plantsPipelineDesc);
	}
}


void IndirectDrawApp::InitDescriptorSets()
{
	m_cbvDescriptorSet = m_skySphereRootSignature->CreateDescriptorSet(0);
	m_cbvDescriptorSet->SetCBV(0, m_vsConstantBuffer);

	m_groundSrvDescriptorSet = m_groundRootSignature->CreateDescriptorSet(1);
	m_groundSrvDescriptorSet->SetSRV(0, m_groundTexture);

	m_plantsSrvDescriptorSet = m_plantsRootSignature->CreateDescriptorSet(1);
	m_plantsSrvDescriptorSet->SetSRV(0, m_plantsTextureArray);
}


void IndirectDrawApp::UpdateConstantBuffer()
{
	m_vsConstants.projectionMatrix = m_camera.GetProjectionMatrix();
	m_vsConstants.modelViewMatrix = m_camera.GetViewMatrix();
	m_vsConstantBuffer->Update(sizeof(VSConstants), &m_vsConstants);
}


void IndirectDrawApp::LoadAssets()
{
	// Sky sphere
	{
		auto vertexLayout = VertexLayout<VertexComponent::PositionTexcoord>();

		m_skySphereModel = LoadModel("sphere.gltf", vertexLayout);
	}

	// Ground plane and plants
	{
		auto vertexLayout = VertexLayout<VertexComponent::PositionNormalColorTexcoord>();

		m_groundModel = LoadModel("plane_circle.gltf", vertexLayout);
		m_plantsModel = LoadModel("plants.gltf", vertexLayout);
	}

	m_groundTexture = LoadTexture("ground_dry_rgba.ktx");
	m_plantsTextureArray = LoadTexture("texturearray_plants_rgba.ktx");
}


void IndirectDrawApp::InitIndirectArgs()
{
	vector<DrawIndexedIndirectArgs> indirectArgs;

	uint32_t m = 0;
	for (const auto& mesh : m_plantsModel->meshes)
	{
		DrawIndexedIndirectArgs args{
			.indexCountPerInstance	= mesh->meshParts[0].indexCount,
			.instanceCount			= m_objectInstanceCount,
			.startIndexLocation		= mesh->meshParts[0].indexBase,
			.startInstanceLocation	= m * m_objectInstanceCount
		};

		indirectArgs.push_back(args);

		m_numObjects += args.instanceCount;
		++m;
	}

	m_indirectArgsBuffer = CreateDrawIndexedIndirectArgsBuffer("Indirect Args Buffer", indirectArgs.size(), indirectArgs.data());
}


void IndirectDrawApp::InitInstanceData()
{
	RandomNumberGenerator rng;

	vector<InstanceData> instanceData{ m_numObjects };

	for (uint32_t i = 0; i < m_numObjects; ++i)
	{
		float theta = XM_2PI * rng.NextFloat();
		float phi = acosf(1.0f - 2.0f * rng.NextFloat());

		Vector3 position = m_plantRadius * Vector3(sinf(phi) * cosf(theta), 0.0f, cosf(phi));
		instanceData[i].position[0] = position.GetX();
		instanceData[i].position[1] = position.GetY();
		instanceData[i].position[2] = position.GetZ();

		Vector3 rotation = Vector3(0.0f, XM_PI * rng.NextFloat(), 0.0f);
		instanceData[i].rotation[0] = rotation.GetX();
		instanceData[i].rotation[1] = rotation.GetY();
		instanceData[i].rotation[2] = rotation.GetZ();
		
		instanceData[i].scale = 1.0f + 2.0f * rng.NextFloat();
		instanceData[i].texIndex = i / m_objectInstanceCount;
	}

	m_instanceBuffer = CreateVertexBuffer("Instance Buffer", instanceData.size(), sizeof(InstanceData), instanceData.data());
}