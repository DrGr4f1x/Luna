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

#include "PipelineStatisticsApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"
#include "Graphics\DeviceManager.h"

using namespace Luna;
using namespace Math;
using namespace std;


PipelineStatisticsApp::PipelineStatisticsApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{}


int PipelineStatisticsApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void PipelineStatisticsApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void PipelineStatisticsApp::Startup()
{
	// Application initialization, after device creation
}


void PipelineStatisticsApp::Shutdown()
{
	// Application cleanup on shutdown
}


void PipelineStatisticsApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	// Get statistics from the readback buffer
	const uint32_t numSwapChainBuffers = m_deviceManager->GetNumSwapChainBuffers();
	uint32_t resultFrame = (m_deviceManager->GetActiveFrame() + 1) % numSwapChainBuffers;
	bool getResults = GetFrameNumber() >= numSwapChainBuffers;

	if (getResults)
	{
		PipelineStatistics* data = (PipelineStatistics*)m_readbackBuffer->Map();

		m_statistics = data[resultFrame];

		m_readbackBuffer->Unmap();
	}

	UpdateConstantBuffer();
}


void PipelineStatisticsApp::UpdateUI()
{
	if (m_uiOverlay->Header("Settings"))
	{
		m_uiOverlay->ComboBox("Object type", &m_curModel, m_modelNames);
		m_uiOverlay->SliderInt("Grid size", &m_gridSize, 1, 10);

		// Options that can cause a pipeline rebuild
		bool rebuildPipeline = false;
		vector<string> cullModes = { "None", "Back", "Front" };
		rebuildPipeline |= m_uiOverlay->ComboBox("Cull mode", &m_cullMode, cullModes);
		rebuildPipeline |= m_uiOverlay->CheckBox("Blending", &m_blendingEnabled);
		rebuildPipeline |= m_uiOverlay->CheckBox("Discard", &m_discardEnabled);
		rebuildPipeline |= m_uiOverlay->CheckBox("Wireframe", &m_wireframeEnabled);
		rebuildPipeline |= m_uiOverlay->CheckBox("Tessellation", &m_tessellationEnabled);

		if (rebuildPipeline)
		{
			m_deviceManager->WaitForGpu();
			InitPipeline();
		}

		if (m_uiOverlay->Header("Pipeline statistics"))
		{
			m_uiOverlay->Text("Input Assembler Vertices         : %d", m_statistics.IAVertices);
			m_uiOverlay->Text("Input Assembler Primitives       : %d", m_statistics.IAPrimitives);
			m_uiOverlay->Text("Vertex Shader Invocations        : %d", m_statistics.VSInvocations);
			m_uiOverlay->Text("Geometry Shader Invocations      : %d", m_statistics.GSInvocations);
			m_uiOverlay->Text("Geometry Shader Primitives       : %d", m_statistics.GSPrimitives);
			m_uiOverlay->Text("Clipping Invocations             : %d", m_statistics.CInvocations);
			m_uiOverlay->Text("Clipping Primitives              : %d", m_statistics.CPrimitives);
			m_uiOverlay->Text("Pixel Shader Invocations         : %d", m_statistics.PSInvocations);
			m_uiOverlay->Text("Hull Shader Invocations          : %d", m_statistics.HSInvocations);
			m_uiOverlay->Text("Domain Shader Invocations        : %d", m_statistics.DSInvocations);
			m_uiOverlay->Text("Compute Shader Invocations       : %d", m_statistics.CSInvocations);
			m_uiOverlay->Text("Amplification Shader Invocations : %d", m_statistics.ASInvocations);
			m_uiOverlay->Text("Mesh Shader Invocations          : %d", m_statistics.MSInvocations);
			m_uiOverlay->Text("Mesh Shader Primitives           : %d", m_statistics.MSPrimitives);
		}
	}
}


void PipelineStatisticsApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(GetDepthBuffer(), ResourceState::DepthWrite);
	context.ClearColor(GetColorBuffer());
	context.ClearDepthAndStencil(GetDepthBuffer());

	const auto activeFrame = m_deviceManager->GetActiveFrame();

	context.ResetQueries(m_queryHeap, activeFrame, 1);

	context.BeginRendering(GetColorBuffer(), GetDepthBuffer());

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());
	context.SetRootSignature(m_rootSignature);

	context.SetGraphicsPipeline(m_pipeline);

	context.SetDescriptors(0, m_cbvDescriptorSet);

	// Start the pipeline statistics query
	context.BeginQuery(m_queryHeap, activeFrame);

	// Draw a grid of models
	const float deltaZ = 1.0f;
	float z = -0.5f * (float)(m_gridSize - 1);
	for (int32_t j = 0; j < m_gridSize; ++j)
	{
		const float deltaX = 1.0f;
		float x = -0.5f * (float)(m_gridSize - 1);
		for (int32_t i = 0; i < m_gridSize; ++i)
		{
			Vector4 position{ 2.5f * x, 0.0f, 2.5f * z, 0.0f };
			context.SetConstantArray(1, 4, &position);

			m_models[m_curModel]->Render(context);

			x += deltaX;
		}
		z += deltaZ;
	}

	context.EndQuery(m_queryHeap, activeFrame);

	RenderUI(context);

	context.EndRendering();

	// Copy query results to buffer
	context.ResolveQueries(m_queryHeap, activeFrame, 1, m_readbackBuffer, activeFrame * m_queryHeap->GetQuerySize());

	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void PipelineStatisticsApp::CreateDeviceDependentResources()
{
	// Create any resources that depend on the device, but not the window size
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		512.0f);
	Vector3 cameraPosition{ -3.0f, 1.0f, -2.75f };
	m_camera.SetPosition(cameraPosition);
	
	m_controller.RefreshFromCamera();
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.SetOrbitTarget(Vector3(0.0f, 0.0f, 0.0f), 10.0f, 0.25f);
	m_controller.SlowMovement(true);
	m_controller.SlowRotation(false);
	m_controller.SetSpeedScale(0.25f);

	InitRootSignature();
	
	m_vsConstantBuffer = CreateConstantBuffer("VS Constant Buffer", 1, sizeof(VSConstants));

	InitQueryHeap();
	LoadAssets();
	InitDescriptorSet();
}


void PipelineStatisticsApp::CreateWindowSizeDependentResources()
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


void PipelineStatisticsApp::InitRootSignature()
{
	auto rootSignatureDesc = RootSignatureDesc{
		.name				= "Root Signature",
		.flags				= RootSignatureFlags::AllowInputAssemblerInputLayout,
		.rootParameters		= {
			RootCBV(0, ShaderStage::Vertex),
			RootConstants(1, 4, ShaderStage::Vertex)
		}
	};

	m_rootSignature = CreateRootSignature(rootSignatureDesc);
}


void PipelineStatisticsApp::InitPipeline()
{
	auto vertexLayout = VertexLayout<VertexComponent::PositionNormalColor>();

	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= vertexLayout.GetSizeInBytes(),
		.inputClassification	= InputClassification::PerVertexData
	};

	// Select rasterizer state based on UI options
	RasterizerStateDesc rasterizerState{};
	if (m_wireframeEnabled)
	{
		rasterizerState = CommonStates::RasterizerWireframe();
	}
	else
	{
		switch (m_cullMode)
		{
		case 0: rasterizerState = CommonStates::RasterizerTwoSided(); break;
		case 1: rasterizerState = CommonStates::RasterizerDefault(); break;
		case 2: rasterizerState = CommonStates::RasterizerDefaultCW(); break;
		}
	}
	rasterizerState.rasterizerDiscardEnable = m_discardEnabled;

	DepthStencilStateDesc depthStencilState = CommonStates::DepthStateReadWriteReversed();

	// Select blend state based on UI options
	BlendStateDesc blendState = CommonStates::BlendDisable();
	if (m_blendingEnabled)
	{
		blendState = CommonStates::BlendTraditional();
		blendState.renderTargetBlend[0].srcBlendAlpha = Blend::InvSrcAlpha;
		blendState.renderTargetBlend[0].dstBlendAlpha = Blend::Zero;
		depthStencilState = CommonStates::DepthStateReadOnlyReversed();
	}

	// Select tessellation shaders based on UI options
	ShaderNameAndEntry hullShader{};
	ShaderNameAndEntry domainShader{};
	PrimitiveTopology topology = PrimitiveTopology::TriangleList;
	if (m_tessellationEnabled)
	{
		hullShader.shaderFile = "SceneHS";
		domainShader.shaderFile = "SceneDS";
		topology = PrimitiveTopology::PatchList_3_ControlPoint;
	}

	// Graphics pipeline
	GraphicsPipelineDesc pipelineDesc{
		.name				= "Default Graphics PSO",
		.blendState			= blendState,
		.depthStencilState	= depthStencilState,
		.rasterizerState	= rasterizerState,
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= topology,
		.vertexShader		= { .shaderFile = "SceneVS" },
		.pixelShader		= { .shaderFile = "ScenePS" },
		.hullShader			= hullShader,
		.domainShader		= domainShader,
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= vertexLayout.GetElements(),
		.rootSignature		= m_rootSignature
	};
	m_pipeline = CreateGraphicsPipeline(pipelineDesc);
}


void PipelineStatisticsApp::InitQueryHeap()
{
	QueryHeapDesc queryHeapDesc{
		.name			= "PipelineStats Query Heap",
		.type			= QueryHeapType::PipelineStats,
		.queryCount		= m_deviceManager->GetNumSwapChainBuffers()
	};
	m_queryHeap = CreateQueryHeap(queryHeapDesc);

	GpuBufferDesc readbackBufferDesc{
		.name			= "Readback Buffer",
		.resourceType	= ResourceType::ReadbackBuffer,
		.memoryAccess	= MemoryAccess::GpuReadWrite | MemoryAccess::CpuRead,
		.elementCount	= m_deviceManager->GetNumSwapChainBuffers(),
		.elementSize	= m_queryHeap->GetQuerySize()
	};
	m_readbackBuffer = CreateGpuBuffer(readbackBufferDesc);
}


void PipelineStatisticsApp::InitDescriptorSet()
{
	m_cbvDescriptorSet = m_rootSignature->CreateDescriptorSet(0);
	m_cbvDescriptorSet->SetCBV(0, m_vsConstantBuffer);
}


void PipelineStatisticsApp::UpdateConstantBuffer()
{
	m_vsConstants.projectionMatrix = m_camera.GetProjectionMatrix();
	m_vsConstants.modelViewMatrix = m_camera.GetViewMatrix();
	m_vsConstants.lightPos = Vector4(-10.0f, 10.0f, 10.0f, 1.0f);

	m_vsConstantBuffer->Update(sizeof(VSConstants), &m_vsConstants);
}


void PipelineStatisticsApp::LoadAssets()
{
	auto layout = VertexLayout<VertexComponent::PositionNormalColor>();

	m_modelNames = { "Sphere", "Teapot", "Torus Knot", "Venus" };
	vector<string> modelFiles = { "sphere.gltf", "teapot.gltf", "torusknot.gltf", "venus.gltf" };
	vector<float> modelScales = { 1.0f, 0.5f, 1.0f, 1.6f };

	m_models.clear();
	m_models.reserve(modelFiles.size());

	uint32_t i = 0;
	for (const string& file : modelFiles)
	{
		auto model = LoadModel(file, layout, modelScales[i]);
		m_models.push_back(model);
		++i;
	}
}