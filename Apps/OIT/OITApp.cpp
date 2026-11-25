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

#include "OITApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"

using namespace Luna;
using namespace Math;
using namespace std;


OITApp::OITApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{
}


int OITApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void OITApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void OITApp::Startup()
{
	// Application initialization, after device creation
}


void OITApp::Shutdown()
{
	// Application cleanup on shutdown
}


void OITApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffer();
}


void OITApp::Render()
{
	// Application main render loop
	Application::Render();
}


void OITApp::CreateDeviceDependentResources()
{
	m_sceneConstantBuffer = CreateConstantBuffer("Scene Constant Buffer", 1, sizeof(SceneConstants));

	InitRootSignatures();
	LoadAssets();
}


void OITApp::CreateWindowSizeDependentResources()
{
	if (!m_pipelinesCreated)
	{
		InitGraphicsPipelines();
		m_pipelinesCreated = true;
	}

	InitLinkedListData();

	m_camera.SetAspectRatio(GetWindowAspectRatio());
}


void OITApp::InitRootSignatures()
{
	RootSignatureDesc geometryDesc{
		.name = "Geometry Root Signature",
		.rootParameters = {
			RootCBV(0, ShaderStage::Vertex),
			RootConstants(1, (uint32_t)SizeOfInUint32<PushConstants>(), ShaderStage::Vertex | ShaderStage::Pixel),
			Table({ StructuredBufferUAV(0, 2), TextureUAV }, ShaderStage::Pixel)
		}
	};

	m_geometryRootSignature = CreateRootSignature(geometryDesc);

	RootSignatureDesc colorDesc{
		.name = "Color Root Signature",
		.rootParameters = {
			Table({ TextureUAV, StructuredBufferUAV }, ShaderStage::Pixel)
		}
	};

	m_colorRootSignature = CreateRootSignature(colorDesc);
}


void OITApp::InitGraphicsPipelines()
{
	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= 3 * sizeof(float),
		.inputClassification	= InputClassification::PerVertexData
	};

	auto layout = VertexLayout<VertexComponent::Position>();

	// Geometry pipeline
	GraphicsPipelineDesc geometryPipelineDesc{
		.name				= "Geometry Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadOnly(),
		.rasterizerState	= CommonStates::RasterizerDefault(),
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "GeometryVS" },
		.pixelShader		= { .shaderFile = "GeometryPS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= layout.GetElements(),
		.rootSignature		= m_geometryRootSignature
	};

	m_geometryPipeline = CreateGraphicsPipeline(geometryPipelineDesc);

	// Color pipeline
	GraphicsPipelineDesc colorPipelineDesc{
		.name				= "Color Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadOnly(),
		.rasterizerState	= CommonStates::RasterizerDefault(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "ColorVS" },
		.pixelShader		= { .shaderFile = "ColorPS" },
		.rootSignature		= m_colorRootSignature
	};

	m_colorPipeline = CreateGraphicsPipeline(colorPipelineDesc);
}

void OITApp::InitLinkedListData()
{
	m_geometryInfo.count = 0;
	m_geometryInfo.maxNodeCount = m_nodeCount * GetWindowWidth() * GetWindowHeight();

	GpuBufferDesc geometryInfoDesc{
		.name					= "Geometry Info Buffer",
		.resourceType			= ResourceType::StructuredBuffer,
		.memoryAccess			= MemoryAccess::GpuReadWrite,
		.elementCount			= 1,
		.elementSize			= sizeof(GeometryInfo),
		.initialData			= &m_geometryInfo,
		.bAllowUnorderedAccess	= true
	};
	m_geometryInfoBuffer = CreateGpuBuffer(geometryInfoDesc);

	GpuBufferDesc linkedListDesc{
		.name					= "Linked List Buffer",
		.resourceType			= ResourceType::StructuredBuffer,
		.memoryAccess			= MemoryAccess::GpuReadWrite,
		.elementCount			= m_geometryInfo.maxNodeCount,
		.elementSize			= sizeof(Node),
		.bAllowUnorderedAccess	= true
	};
	m_linkedListBuffer = CreateGpuBuffer(linkedListDesc);

	ColorBufferDesc listHeadDesc{
		.name = "List Head Texture",
		.resourceType = ResourceType::Texture2D,
		.width = GetWindowWidth(),
		.height = GetWindowHeight(),
		.format = Format::R32_UInt,
	};
	m_listHeadTexture = CreateColorBuffer(listHeadDesc);
}


void OITApp::InitDescriptorSets()
{
	m_geometryDescriptorSet = m_geometryRootSignature->CreateDescriptorSet(2,
		{ m_geometryInfoBuffer, 
		  m_linkedListBuffer, 
		  m_listHeadTexture });

	m_colorDescriptorSet = m_colorRootSignature->CreateDescriptorSet(0,
		{ m_listHeadTexture,
		  m_linkedListBuffer });
}


void OITApp::UpdateConstantBuffer()
{
	m_sceneConstants.viewMatrix = m_camera.GetViewMatrix();
	m_sceneConstants.projectionMatrix = m_camera.GetProjectionMatrix();

	m_sceneConstantBuffer->Update(sizeof(m_sceneConstants), &m_sceneConstants);
}


void OITApp::LoadAssets()
{
	auto layout = VertexLayout<VertexComponent::Position>();
	m_cubeModel = LoadModel("cube.gltf", layout);
	m_sphereModel = LoadModel("sphere.gltf", layout);
}