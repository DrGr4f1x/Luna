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

#include "OcclusionQueryApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"
#include "Graphics\DeviceManager.h"

using namespace Luna;
using namespace Math;
using namespace std;


OcclusionQueryApp::OcclusionQueryApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{}


int OcclusionQueryApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void OcclusionQueryApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void OcclusionQueryApp::Startup()
{
	// Application initialization, after device creation
}


void OcclusionQueryApp::Shutdown()
{
	// Application cleanup on shutdown
}


void OcclusionQueryApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffers();
}


void OcclusionQueryApp::UpdateUI()
{
	if (m_uiOverlay->Header("Occlusion query results"))
	{
		m_uiOverlay->Text("Teapot: %d samples passed", m_passedSamples[0]);
		m_uiOverlay->Text("Sphere: %d samples passed", m_passedSamples[1]);
	}
}


void OcclusionQueryApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(GetDepthBuffer(), ResourceState::DepthWrite);
	context.ClearColor(GetColorBuffer());
	context.ClearDepthAndStencil(GetDepthBuffer());

	const auto activeFrame = m_deviceManager->GetActiveFrame();

	// Occlusion pass

	context.ResetQueries(m_queryHeap, 2 * activeFrame, 2);
	context.BeginRendering(GetColorBuffer(), GetDepthBuffer());

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());
	context.SetRootSignature(m_rootSignature);

	{
		context.SetGraphicsPipeline(m_simplePipeline);

		// Occluder plane
		context.SetRootCBV(0, m_occluderConstantBuffer);
		m_occluderModel->Render(context);

		// Teapot
		context.BeginQuery(m_queryHeap, 2 * activeFrame);

		context.SetRootCBV(0, m_teapotConstantBuffer);
		m_teapotModel->Render(context);

		context.EndQuery(m_queryHeap, 2 * activeFrame);

		// Sphere
		context.BeginQuery(m_queryHeap, 2 * activeFrame + 1);

		context.SetRootCBV(0, m_sphereConstantBuffer);
		m_sphereModel->Render(context);

		context.EndQuery(m_queryHeap, 2 * activeFrame + 1);
	}
	context.EndRendering();

	// Copy query results to buffer
	context.ResolveQueries(m_queryHeap, 2 * activeFrame, 2, m_readbackBuffer, 2 * activeFrame * sizeof(uint64_t));

	context.ClearColor(GetColorBuffer());
	context.ClearDepthAndStencil(GetDepthBuffer());

	// Visible pass

	context.BeginRendering(GetColorBuffer(), GetDepthBuffer());

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());
	context.SetRootSignature(m_rootSignature);
	{
		// Teapot
		context.SetGraphicsPipeline(m_solidPipeline);
		context.SetRootCBV(0, m_teapotConstantBuffer);
		m_teapotModel->Render(context);

		// Sphere
		context.SetRootCBV(0, m_sphereConstantBuffer);
		m_sphereModel->Render(context);

		// Occluder plane
		context.SetGraphicsPipeline(m_occluderPipeline);
		context.SetRootCBV(0, m_occluderConstantBuffer);
		m_occluderModel->Render(context);
	}

	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void OcclusionQueryApp::CreateDeviceDependentResources()
{
	m_camera.SetPosition(Vector3(0.0f, 0.0f, m_zoom));

	m_controller.SetSpeedScale(0.01f);
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.RefreshFromCamera();
	m_controller.SetOrbitTarget(Vector3(0.0f, 0.0f, 0.0f), Length(m_camera.GetPosition()), 0.25f);

	InitRootSignature();
	
	// Create constant buffers
	m_occluderConstantBuffer = CreateConstantBuffer("Occluder Constant Buffer", 1, sizeof(VSConstants));
	m_teapotConstantBuffer = CreateConstantBuffer("Teapot Constant Buffer", 1, sizeof(VSConstants));
	m_sphereConstantBuffer = CreateConstantBuffer("Sphere Constant Buffer", 1, sizeof(VSConstants));

	InitQueryHeap();
	LoadAssets();
}


void OcclusionQueryApp::CreateWindowSizeDependentResources()
{
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

	m_controller.SetSpeedScale(0.01f);
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.SetOrbitTarget(Vector3(0.0f, 0.0f, 0.0f), Length(m_camera.GetPosition()), 0.25f);
}


void OcclusionQueryApp::InitRootSignature()
{
	auto rootSignatureDesc = RootSignatureDesc{
		.name				= "Root Signature",
		.rootParameters		= {	RootCBV(0, ShaderStage::Vertex) }
	};

	m_rootSignature = CreateRootSignature(rootSignatureDesc);
}


void OcclusionQueryApp::InitPipelines()
{
	auto layout = VertexLayout<VertexComponent::PositionNormalColor>();

	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= layout.GetSizeInBytes(),
		.inputClassification	= InputClassification::PerVertexData
	};

	// Solid pipeline
	GraphicsPipelineDesc solidDesc{
		.name				= "Solid Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState	= CommonStates::RasterizerDefault(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "MeshVS" },
		.pixelShader		= { .shaderFile = "MeshPS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= layout.GetElements(),
		.rootSignature		= m_rootSignature
	};

	m_solidPipeline = CreateGraphicsPipeline(solidDesc);

	// Simple pipeline
	GraphicsPipelineDesc simpleDesc = solidDesc;
	simpleDesc.SetName("Simple Graphics PSO");
	simpleDesc.SetRasterizerState(CommonStates::RasterizerTwoSided());
	simpleDesc.SetVertexShader("SimpleVS");
	simpleDesc.SetPixelShader("SimplePS");

	m_simplePipeline = CreateGraphicsPipeline(simpleDesc);

	// Occluder pipeline
	GraphicsPipelineDesc occluderDesc = solidDesc;
	occluderDesc.SetName("Occluder Graphics PSO");
	BlendStateDesc blendDesc{};
	blendDesc.renderTargetBlend[0].blendEnable = true;
	blendDesc.renderTargetBlend[0].blendOp = BlendOp::Add;
	blendDesc.renderTargetBlend[0].srcBlend = Blend::SrcColor;
	blendDesc.renderTargetBlend[0].dstBlend = Blend::InvSrcColor;
	occluderDesc.SetBlendState(blendDesc);
	occluderDesc.SetRasterizerState(CommonStates::RasterizerTwoSided());
	occluderDesc.SetVertexShader("OccluderVS");
	occluderDesc.SetPixelShader("OccluderPS");

	m_occluderPipeline = CreateGraphicsPipeline(occluderDesc);
}


void OcclusionQueryApp::InitQueryHeap()
{
	QueryHeapDesc queryHeapDesc{
		.name			= "Occlusion Query Heap",
		.type			= QueryHeapType::Occlusion,
		.queryCount		= 2 * m_deviceManager->GetNumSwapChainBuffers()
	};
	m_queryHeap = CreateQueryHeap(queryHeapDesc);

	GpuBufferDesc readbackBufferDesc{
		.name			= "Readback Buffer",
		.resourceType	= ResourceType::ReadbackBuffer,
		.memoryAccess	= MemoryAccess::GpuReadWrite | MemoryAccess::CpuRead,
		.elementCount	= 2 * m_deviceManager->GetNumSwapChainBuffers(),
		.elementSize	= sizeof(uint64_t)
	};
	m_readbackBuffer = CreateGpuBuffer(readbackBufferDesc);
}


void OcclusionQueryApp::UpdateConstantBuffers()
{
	using namespace DirectX;

	Matrix4 projectionMatrix = m_camera.GetProjectionMatrix();
	Matrix4 viewMatrix = m_camera.GetViewMatrix();

	Matrix4 rotationMatrix(AffineTransform::MakeYRotation(XMConvertToRadians(-135.0f)));

	m_occluderConstants.projectionMatrix = projectionMatrix;
	m_occluderConstants.modelViewMatrix = viewMatrix * rotationMatrix;
	m_occluderConstants.color = DirectX::Colors::Blue;
	m_occluderConstantBuffer->Update(sizeof(m_occluderConstants), &m_occluderConstants);

	uint64_t teapotQueryResult = 1;
	uint64_t sphereQueryResult = 1;

	const uint32_t numSwapChainBuffers = m_deviceManager->GetNumSwapChainBuffers();
	uint32_t resultFrame = (m_deviceManager->GetActiveFrame() + 1) % numSwapChainBuffers;
	bool getResults = GetFrameNumber() >= numSwapChainBuffers;

	if (getResults)
	{
		uint64_t* data = (uint64_t*)m_readbackBuffer->Map();

		teapotQueryResult = data[2 * resultFrame];
		sphereQueryResult = data[2 * resultFrame + 1];

		m_passedSamples[0] = (uint32_t)teapotQueryResult;
		m_passedSamples[1] = (uint32_t)sphereQueryResult;

		m_readbackBuffer->Unmap();
	}

	m_teapotConstants.projectionMatrix = projectionMatrix;
	m_teapotConstants.modelViewMatrix = viewMatrix * rotationMatrix * Matrix4(AffineTransform::MakeTranslation(Vector3(0.0f, 0.0f, -3.0f)));
	m_teapotConstants.visible = teapotQueryResult > 0 ? 1.0f : 0.0f;
	m_teapotConstants.color = DirectX::Colors::Red;
	m_teapotConstantBuffer->Update(sizeof(m_teapotConstants), &m_teapotConstants);

	m_sphereConstants.projectionMatrix = projectionMatrix;
	m_sphereConstants.modelViewMatrix = viewMatrix * rotationMatrix * Matrix4(AffineTransform::MakeTranslation(Vector3(0.0f, 0.0f, 3.0f)));
	m_sphereConstants.visible = sphereQueryResult > 0 ? 1.0f : 0.0f;
	m_sphereConstants.color = DirectX::Colors::Green;
	m_sphereConstantBuffer->Update(sizeof(m_sphereConstants), &m_sphereConstants);
}


void OcclusionQueryApp::LoadAssets()
{
	auto layout = VertexLayout<VertexComponent::PositionNormalColor>();

	m_occluderModel = LoadModel("plane_z.gltf", layout, 7.0f);
	m_teapotModel = LoadModel("teapot.gltf", layout, 1.0f);
	m_sphereModel = LoadModel("sphere.gltf", layout, 1.0f);
}