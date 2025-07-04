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

#include "TextureArrayApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"


using namespace Luna;
using namespace Math;
using namespace std;


TextureArrayApp::TextureArrayApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
	, m_controller{ m_camera, Vector3(kYUnitVector) }
{}


int TextureArrayApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void TextureArrayApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void TextureArrayApp::Startup()
{
	// TODO: Split this between CreateDeviceDependentResources() and CreateWindowSizeDependentResources

	// Setup vertices for a single uv-mapped quad made from two triangles
	vector<Vertex> vertexData =
	{
		{ {  2.5f,  2.5f,  0.0f }, { 1.0f, 1.0f } },
		{ { -2.5f,  2.5f,  0.0f }, { 0.0f, 1.0f } },
		{ { -2.5f, -2.5f,  0.0f }, { 0.0f, 0.0f } },
		{ {  2.5f, -2.5f,  0.0f }, { 1.0f, 0.0f } }
	};

	GpuBufferDesc vertexBufferDesc{
		.name			= "Vertex Buffer",
		.resourceType	= ResourceType::VertexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= vertexData.size(),
		.elementSize	= sizeof(Vertex),
		.initialData	= vertexData.data()
	};
	m_vertexBuffer = CreateGpuBuffer(vertexBufferDesc);

	// Setup indices
	vector<uint32_t> indexData = { 0,1,2, 2,3,0 };
	GpuBufferDesc indexBufferDesc{
		.name			= "Index Buffer",
		.resourceType	= ResourceType::IndexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= indexData.size(),
		.elementSize	= sizeof(uint32_t),
		.initialData	= indexData.data()
	};
	m_indexBuffer = CreateGpuBuffer(indexBufferDesc);

	// Setup camera
	m_camera.SetPerspectiveMatrix(
		XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.001f,
		256.0f);
	m_camera.SetPosition(Vector3(0.0f, -1.0f, m_zoom));

	m_camera.Update();

	m_controller.SetSpeedScale(0.01f);
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.SetOrbitTarget(Math::Vector3(0.0f, 0.0f, 0.0f), Length(m_camera.GetPosition()), 0.25f);
	m_controller.RefreshFromCamera();

	InitDepthBuffer();
	InitRootSignature();
	InitGraphicsPipeline();

	// We have to load the texture first, so we know how many array slices there are
	LoadAssets();
	InitConstantBuffer();

	InitResourceSet();
}


void TextureArrayApp::Shutdown()
{
	delete[] m_constants.instance;
}


void TextureArrayApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffer();
}


void TextureArrayApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(m_depthBuffer, ResourceState::DepthWrite);
	context.ClearColor(GetColorBuffer());
	context.ClearDepth(m_depthBuffer);

	context.BeginRendering(GetColorBuffer(), m_depthBuffer);

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());

	context.SetRootSignature(m_rootSignature);
	context.SetGraphicsPipeline(m_graphicsPipeline);

	context.SetResources(m_resources);

	context.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
	context.SetVertexBuffer(0, m_vertexBuffer);
	context.SetIndexBuffer(m_indexBuffer);

	context.DrawIndexedInstanced((uint32_t)m_indexBuffer->GetElementCount(), m_layerCount, 0, 0, 0);

	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void TextureArrayApp::CreateDeviceDependentResources()
{
	// Create any resources that depend on the device, but not the window size
}


void TextureArrayApp::CreateWindowSizeDependentResources()
{
	// Create any resources that depend on window size.  May be called when the window size changes.
}


void TextureArrayApp::InitDepthBuffer()
{
	DepthBufferDesc depthBufferDesc{
		.name			= "Depth Buffer",
		.resourceType	= ResourceType::Texture2D,
		.width			= GetWindowWidth(),
		.height			= GetWindowHeight(),
		.format			= GetDepthFormat()
	};

	m_depthBuffer = CreateDepthBuffer(depthBufferDesc);
}


void TextureArrayApp::InitRootSignature()
{
	auto rootSignatureDesc = RootSignatureDesc{
		.name = "Root Signature",
		.flags = RootSignatureFlags::AllowInputAssemblerInputLayout,
		.rootParameters =
			{
				RootParameter::RootCBV(0, ShaderStage::Vertex),
				RootParameter::Table({ DescriptorRange::TextureSRV(0) }, ShaderStage::Pixel),
				RootParameter::Table({ DescriptorRange::Sampler(0) }, ShaderStage::Pixel)
			}
	};

	m_rootSignature = CreateRootSignature(rootSignatureDesc);
}


void TextureArrayApp::InitGraphicsPipeline()
{
	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= sizeof(Vertex),
		.inputClassification	= InputClassification::PerVertexData
	};

	vector<VertexElementDesc> vertexElements{
		{ "POSITION", 0, Format::RGB32_Float, 0, offsetof(Vertex, position), InputClassification::PerVertexData, 0 },
		{ "TEXCOORD", 0, Format::RG32_Float, 0, offsetof(Vertex, uv), InputClassification::PerVertexData, 0 }
	};

	GraphicsPipelineDesc desc
	{
		.name				= "Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadOnlyReversed(),
		.rasterizerState	= CommonStates::RasterizerDefault(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= {.shaderFile = "InstancingVS" },
		.pixelShader		= {.shaderFile = "InstancingPS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= vertexElements,
		.rootSignature		= m_rootSignature
	};

	m_graphicsPipeline = CreateGraphicsPipelineState(desc);
}


void TextureArrayApp::InitConstantBuffer()
{
	m_constants.instance = new InstanceData[m_layerCount];
	size_t size = sizeof(Matrix4) + (m_layerCount * sizeof(InstanceData));

	// Setup constant buffer
	GpuBufferDesc constantBufferDesc{
		.name			= "Constant Buffer",
		.resourceType	= ResourceType::ConstantBuffer,
		.memoryAccess	= MemoryAccess::GpuRead | MemoryAccess::CpuWrite,
		.elementCount	= 1,
		.elementSize	= sizeof(Matrix4) + (m_layerCount * sizeof(InstanceData)),
		.initialData	= nullptr
	};
	m_constantBuffer = CreateGpuBuffer(constantBufferDesc);

	// Setup the per-instance data
	float offset = 1.5f;
	float center = (m_layerCount * offset) / 2.0f;
	for (uint32_t i = 0; i < m_layerCount; ++i)
	{
		float fi = (float)i;
		auto transform = AffineTransform::MakeTranslation(Vector3(0.0f, (fi + 0.5f) * offset - center, 0.0f));
		transform = transform * AffineTransform::MakeXRotation(DirectX::XMConvertToRadians(-60.0f));
		m_constants.instance[i].modelMatrix = transform;
		m_constants.instance[i].arrayIndex.SetX(static_cast<float>(i));
	}
	m_constantBuffer->Update(m_layerCount * sizeof(InstanceData), sizeof(Matrix4), m_constants.instance);
}


void TextureArrayApp::InitResourceSet()
{
	m_resources.Initialize(m_rootSignature);
	m_resources.SetCBV(0, 0, m_constantBuffer);
	m_resources.SetSRV(1, 0, m_texture);
	m_resources.SetSampler(2, 0, m_sampler);
}

void TextureArrayApp::UpdateConstantBuffer()
{
	m_constants.viewProjectionMatrix = m_camera.GetViewProjMatrix();

	// Just update the vp matrix
	m_constantBuffer->Update(sizeof(Matrix4), &m_constants);
}

void TextureArrayApp::LoadAssets()
{
	m_texture = LoadTexture("texturearray_bc3_unorm.ktx");
	m_layerCount = m_texture->GetArraySize();

	m_sampler = CreateSampler(CommonStates::SamplerLinearClamp());
}