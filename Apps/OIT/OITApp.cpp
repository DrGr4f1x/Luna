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
	auto& context = GraphicsContext::Begin("Frame");


	context.TransitionResource(m_geometryInfoBuffer, ResourceState::UnorderedAccess);
	context.TransitionResource(m_listHeadTexture, ResourceState::UnorderedAccess);
	context.TransitionResource(m_linkedListBuffer, ResourceState::UnorderedAccess);

	context.BindDescriptorHeap(GetDefaultDescriptorHeap());

	// Clear geometry info buffer
	auto& computeContext = context.GetComputeContext();
	computeContext.SetRootSignature(m_clearBufferRootSignature);
	computeContext.SetComputePipeline(m_clearBufferPipeline);
	computeContext.SetRootUAV(0, m_geometryInfoBuffer);
	computeContext.Dispatch(1, 1, 1);

	context.InsertUAVBarrier(m_geometryInfoBuffer);

	// Clear list head texture
	computeContext.SetRootSignature(m_clearColorRootSignature);
	computeContext.SetComputePipeline(m_clearColorPipeline);
	computeContext.SetDescriptors(0, m_clearColorDescriptorSet);
	computeContext.Dispatch2D(GetWindowWidth(), GetWindowHeight(), 8, 8);

	context.InsertUAVBarrier(m_listHeadTexture);

	// Depth pass
	{
		ScopedDrawEvent offscreenEvent(context, "Depth Pass");

		context.BeginRendering(GetWindowWidth(), GetWindowHeight());

		context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());	

		context.SetGraphicsPipeline(m_geometryPipeline);
		context.SetRootSignature(m_geometryRootSignature);

		context.SetRootCBV(0, m_sceneConstantBuffer);
		context.SetDescriptors(2, m_geometryDescriptorSet);

		//context.SetIndexBuffer(m_sphereModel->meshes[0]->indexBuffer);
		//context.SetVertexBuffer(0, m_sphereModel->meshes[0]->vertexBuffer);

		ObjectData objectData{};
		objectData.color = Vector4(1.0f, 0.0f, 0.0f, 0.5f);

		// Spheres
		for (int x = 0; x < 5; ++x)
		{
			for (int y = 0; y < 5; ++y)
			{
				for (int z = 0; z < 5; ++z)
				{
					Matrix4 T = Matrix4::MakeTranslation({ (float)(x - 2), (float)(y - 2), (float)(z - 2) });
					Matrix4 S = Matrix4::MakeScale({ 0.3f, 0.3f, 0.3f });
					objectData.modelMatrix = T * S;

					context.SetConstantArray(1, sizeof(ObjectData) / sizeof(uint32_t), &objectData);
					m_sphereModel->Render(context);
				}
			}
		}

		objectData.color = Vector4(0.0f, 0.0f, 1.0f, 0.5f);
		for (int x = 0; x < 2; ++x)
		{
			Matrix4 T = Matrix4::MakeTranslation({ 3.0f * x - 1.5f, 0.0f, 0.0f });
			Matrix4 S = Matrix4::MakeScale({ 0.2f, 0.2f, 0.2f });
			objectData.modelMatrix = T * S;

			context.SetConstantArray(1, sizeof(ObjectData) / sizeof(uint32_t), &objectData);
			m_cubeModel->Render(context);
		}

		context.EndRendering();
	}

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(m_listHeadTexture, ResourceState::PixelShaderResource);
	context.TransitionResource(m_linkedListBuffer, ResourceState::PixelShaderResource);

	// Color pass
	{
		ScopedDrawEvent offscreenEvent(context, "Color Pass");

		context.BeginRendering(GetColorBuffer());

		context.SetRootSignature(m_colorRootSignature);
		context.SetGraphicsPipeline(m_colorPipeline);
		context.SetDescriptors(0, m_colorDescriptorSet);
		context.Draw(3);

		context.EndRendering();
	}

	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void OITApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		XMConvertToRadians(45.0f),
		GetWindowAspectRatio(),
		0.1f,
		256.0f);
	m_camera.SetPosition(Vector3(0.0f, 0.0f, 6.0f));

	m_sceneConstantBuffer = CreateConstantBuffer("Scene Constant Buffer", 1, sizeof(SceneConstants));

	InitRootSignatures();
	LoadAssets();

	m_controller.SetSpeedScale(0.01f);
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.RefreshFromCamera();
	m_controller.SetOrbitTarget(Vector3(0.0f, 0.0f, 0.0f), Length(m_camera.GetPosition()), 4.0f);
}


void OITApp::CreateWindowSizeDependentResources()
{
	if (!m_pipelinesCreated)
	{
		InitPipelines();
		m_pipelinesCreated = true;
	}

	InitLinkedListData();
	InitDescriptorSets();

	m_camera.SetAspectRatio(GetWindowAspectRatio());
}


void OITApp::InitRootSignatures()
{
	RootSignatureDesc clearColorDesc{
		.name				= "Clear Color Root Signature",
		.rootParameters		= {
			Table({ TextureUAV }, ShaderStage::Compute)
		}
	};

	m_clearColorRootSignature = CreateRootSignature(clearColorDesc);

	RootSignatureDesc clearBufferDesc{
		.name				= "Clear Buffer Root Signature",
		.rootParameters		= { RootUAV(0, ShaderStage::Compute) }
	};

	m_clearBufferRootSignature = CreateRootSignature(clearBufferDesc);

	RootSignatureDesc geometryDesc{
		.name				= "Geometry Root Signature",
		.rootParameters		= {
			RootCBV(0, ShaderStage::Vertex),
			RootConstants(1, sizeof(ObjectData) / sizeof(uint32_t), ShaderStage::Vertex | ShaderStage::Pixel),
			Table({ StructuredBufferUAV(0, 2), TextureUAV }, ShaderStage::Pixel)
		}
	};

	m_geometryRootSignature = CreateRootSignature(geometryDesc);

	RootSignatureDesc colorDesc{
		.name				= "Color Root Signature",
		.rootParameters		= {
			Table({ TextureSRV, StructuredBufferSRV }, ShaderStage::Pixel)
		}
	};

	m_colorRootSignature = CreateRootSignature(colorDesc);
}


void OITApp::InitPipelines()
{
	// Clear color pipeline
	ComputePipelineDesc clearColorDesc{
		.name			= "Clear Color Compute PSO",
		.computeShader	= {.shaderFile = "ClearColorCS" },
		.rootSignature	= m_clearColorRootSignature
	};

	m_clearColorPipeline = CreateComputePipeline(clearColorDesc);

	// Clear buffer pipeline
	ComputePipelineDesc clearBufferDesc{
		.name			= "Clear Buffer Compute PSO",
		.computeShader	= {.shaderFile = "ClearBufferCS" },
		.rootSignature	= m_clearBufferRootSignature
	};

	m_clearBufferPipeline = CreateComputePipeline(clearBufferDesc);

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
		.depthStencilState	= CommonStates::DepthStateDisabled(),
		.rasterizerState	= CommonStates::RasterizerDefault(),
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
		.depthStencilState	= CommonStates::DepthStateDisabled(),
		.rasterizerState	= CommonStates::RasterizerTwoSided(),
		.rtvFormats			= { GetColorFormat() },
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
		.name			= "List Head Texture",
		.resourceType	= ResourceType::Texture2D,
		.width			= GetWindowWidth(),
		.height			= GetWindowHeight(),
		.format			= Format::R32_UInt,
		.clearColor		= Color(~0u)
	};
	m_listHeadTexture = CreateColorBuffer(listHeadDesc);
}


void OITApp::InitDescriptorSets()
{
	m_clearColorDescriptorSet = m_clearColorRootSignature->CreateDescriptorSet(0, m_listHeadTexture);

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