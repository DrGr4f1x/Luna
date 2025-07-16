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

#include "DynamicUniformBufferApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"
#include "Graphics\Limits.h"

using namespace Luna;
using namespace Math;
using namespace std;


DynamicUniformBufferApp::DynamicUniformBufferApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
	, m_controller{ m_camera, Vector3(kYUnitVector) }
{}


int DynamicUniformBufferApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void DynamicUniformBufferApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void DynamicUniformBufferApp::Startup()
{
	// Application initialization, after device creation
}


void DynamicUniformBufferApp::Shutdown()
{
	_aligned_free(m_vsModelConstants.modelMatrix);
}


void DynamicUniformBufferApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffers();
}


void DynamicUniformBufferApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(m_depthBuffer, ResourceState::DepthWrite);
	context.ClearColor(GetColorBuffer());
	context.ClearDepthAndStencil(m_depthBuffer);

	context.BeginRendering(GetColorBuffer(), m_depthBuffer);

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());
	context.SetRootSignature(m_rootSignature);
	context.SetGraphicsPipeline(m_pipeline);

	context.SetIndexBuffer(m_indexBuffer);
	context.SetVertexBuffer(0, m_vertexBuffer);

	for (uint32_t i = 0; i < m_numCubes; ++i)
	{
		uint32_t dynamicOffset = i * (uint32_t)m_dynamicAlignment;
		m_resources.SetDynamicOffset(1, dynamicOffset);

		context.SetResources(m_resources);

		context.DrawIndexed((uint32_t)m_indexBuffer->GetElementCount());
	}

	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void DynamicUniformBufferApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		256.0f);
	m_camera.SetPosition(Vector3(0.0f, 0.0f, -30.0f));
	m_camera.Update();

	m_controller.SetSpeedScale(0.01f);
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.SetOrbitTarget(Vector3(0.0f, 0.0f, 0.0f), Length(m_camera.GetPosition()), 0.25f);

	InitRootSignature();
	InitConstantBuffers();
	InitBox();
	InitResourceSet();
}


void DynamicUniformBufferApp::CreateWindowSizeDependentResources()
{
	InitDepthBuffer();
	if (!m_pipelineCreated)
	{
		InitPipeline();
		m_pipelineCreated = true;
	}

	// Update camera since the aspect ratio might have changed
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		256.0f);
	m_camera.Update();
}


void DynamicUniformBufferApp::InitDepthBuffer()
{
	DepthBufferDesc depthBufferDesc{
		.name	= "Depth Buffer",
		.width	= GetWindowWidth(),
		.height	= GetWindowHeight(),
		.format = GetDepthFormat()
	};

	m_depthBuffer = CreateDepthBuffer(depthBufferDesc);
}


void DynamicUniformBufferApp::InitRootSignature()
{
	RootSignatureDesc rootSignatureDesc{
		.name				= "Root Signature",
		.flags				= RootSignatureFlags::AllowInputAssemblerInputLayout | RootSignatureFlags::DenyPixelShaderRootAccess,
		.rootParameters		= {	
			RootParameter::RootCBV(0, ShaderStage::Vertex),	
			RootParameter::RootCBV(1, ShaderStage::Vertex)
		}
	};

	m_rootSignature = CreateRootSignature(rootSignatureDesc);
}


void DynamicUniformBufferApp::InitPipeline()
{
	auto vertexLayout = VertexLayout<VertexComponent::PositionColor>();

	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= vertexLayout.GetSizeInBytes(),
		.inputClassification	= InputClassification::PerVertexData
	};

	GraphicsPipelineDesc pipelineDesc{
		.name				= "Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState	= CommonStates::RasterizerTwoSided(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "BaseVS" },
		.pixelShader		= { .shaderFile = "BasePS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= vertexLayout.GetElements(),
		.rootSignature		= m_rootSignature
	};

	m_pipeline = CreateGraphicsPipeline(pipelineDesc);
}


void DynamicUniformBufferApp::InitConstantBuffers()
{
	GpuBufferDesc desc{
		.name			= "VS Constant Buffer",
		.resourceType	= ResourceType::ConstantBuffer,
		.memoryAccess	= MemoryAccess::GpuRead | MemoryAccess::CpuWrite,
		.elementCount	= 1,
		.elementSize	= sizeof(VSConstants)
	};
	m_vsConstantBuffer = CreateGpuBuffer(desc);

	m_dynamicAlignment = AlignUp(sizeof(Matrix4), Limits::ConstantBufferAlignment());

	size_t allocSize = m_numCubes * m_dynamicAlignment;
	m_vsModelConstants.modelMatrix = (Matrix4*)_aligned_malloc(allocSize, m_dynamicAlignment);

	GpuBufferDesc modelDesc{
		.name				= "VS Model Constant Buffer",
		.resourceType		= ResourceType::ConstantBuffer,
		.memoryAccess		= MemoryAccess::GpuRead | MemoryAccess::CpuWrite,
		.elementCount		= m_numCubes,
		.elementSize		= m_dynamicAlignment
	};
	m_vsModelConstantBuffer = CreateGpuBuffer(modelDesc);
}


void DynamicUniformBufferApp::InitBox()
{
	struct Vertex
	{
		float pos[3];
		float color[4];
	};

	vector<Vertex> vertices =
	{
		{ { -1.0f, -1.0f,  1.0f },{ 1.0f, 0.0f, 0.0f, 1.0f } },
		{ {  1.0f, -1.0f,  1.0f },{ 0.0f, 1.0f, 0.0f, 1.0f } },
		{ {  1.0f,  1.0f,  1.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { -1.0f,  1.0f,  1.0f },{ 0.0f, 0.0f, 0.0f, 1.0f } },
		{ { -1.0f, -1.0f, -1.0f },{ 1.0f, 0.0f, 0.0f, 1.0f } },
		{ {  1.0f, -1.0f, -1.0f },{ 0.0f, 1.0f, 0.0f, 1.0f } },
		{ {  1.0f,  1.0f, -1.0f },{ 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { -1.0f,  1.0f, -1.0f },{ 0.0f, 0.0f, 0.0f, 1.0f } },
	};

	GpuBufferDesc vertexBufferDesc{
		.name			= "Vertex Buffer",
		.resourceType	= ResourceType::VertexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= vertices.size(),
		.elementSize	= sizeof(Vertex),
		.initialData	= vertices.data()
	};

	m_vertexBuffer = CreateGpuBuffer(vertexBufferDesc);

	vector<uint16_t> indices =
	{
		0,1,2, 2,3,0, // Face 0
		1,5,6, 6,2,1, // ...
		7,6,5, 5,4,7, // ...
		4,0,3, 3,7,4, // ...
		4,5,1, 1,0,4, // ...
		3,2,6, 6,7,3, // Face 5
	};

	GpuBufferDesc indexBufferDesc{
		.name			= "Index Buffer",
		.resourceType	= ResourceType::IndexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= indices.size(),
		.elementSize	= sizeof(uint16_t),
		.initialData	= indices.data()
	};

	m_indexBuffer = CreateGpuBuffer(indexBufferDesc);
}


void DynamicUniformBufferApp::InitResourceSet()
{
	m_resources.Initialize(m_rootSignature);
	m_resources.SetCBV(0, 0, m_vsConstantBuffer);
	m_resources.SetCBV(1, 0, m_vsModelConstantBuffer);
}


void DynamicUniformBufferApp::UpdateConstantBuffers()
{
	using namespace Math;

	m_vsConstants.projectionMatrix = m_camera.GetProjMatrix();
	m_vsConstants.viewMatrix = m_camera.GetViewMatrix();

	m_vsConstantBuffer->Update(sizeof(m_vsConstants), &m_vsConstants);

	m_animationTimer += (float)m_timer.GetElapsedSeconds();
	if (m_animationTimer <= 1.0f / 60.0f)
		return;

	Vector3 offset(5.0f);
	const float dim = static_cast<float>(m_numCubesSide);

	for (uint32_t x = 0; x < m_numCubesSide; ++x)
	{
		for (uint32_t y = 0; y < m_numCubesSide; ++y)
		{
			for (uint32_t z = 0; z < m_numCubesSide; ++z)
			{
				const uint32_t index = x * m_numCubesSide * m_numCubesSide + y * m_numCubesSide + z;

				auto modelMatrix = (Matrix4*)((uint64_t)m_vsModelConstants.modelMatrix + (index * m_dynamicAlignment));

				m_rotations[index] += m_animationTimer * m_rotationSpeeds[index];

				Vector3 pos = {
					-((dim * offset.GetX()) / 2.0f) + offset.GetX() / 2.0f + (float)x * offset.GetX(),
					-((dim * offset.GetY()) / 2.0f) + offset.GetY() / 2.0f + (float)y * offset.GetY(),
					-((dim * offset.GetZ()) / 2.0f) + offset.GetZ() / 2.0f + (float)z * offset.GetZ() };

				Quaternion rotX{ Vector3(1.0f, 1.0f, 0.0f), m_rotations[index].GetX() };
				Quaternion rotY{ Vector3(kYUnitVector), m_rotations[index].GetY() };
				Quaternion rotZ{ Vector3(kZUnitVector), m_rotations[index].GetZ() };
				Quaternion rotCombined = rotZ * rotY * rotX;

				*modelMatrix = AffineTransform{ rotCombined, pos };
			}
		}
	}

	m_vsModelConstantBuffer->Update(m_dynamicAlignment * m_numCubes, m_vsModelConstants.modelMatrix);

	m_animationTimer = 0.0f;
}