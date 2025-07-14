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

#include "ComputeClothApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"

using namespace Luna;
using namespace Math;
using namespace std;


ComputeClothApp::ComputeClothApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
	, m_controller{ m_camera, Vector3(kYUnitVector) }
{}


int ComputeClothApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void ComputeClothApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void ComputeClothApp::Startup()
{
	// Application initialization, after device creation
}


void ComputeClothApp::Shutdown()
{
	// Application cleanup on shutdown
}


void ComputeClothApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffers();
}


void ComputeClothApp::UpdateUI()
{
	if (m_uiOverlay->Header("Settings"))
	{
		m_uiOverlay->CheckBox("Simulate wind", &m_simulateWind);
	}
}


void ComputeClothApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	// Cloth simulation
	{
		auto& computeContext = context.GetComputeContext();

		computeContext.TransitionResource(m_clothBuffer[0], ResourceState::ShaderResource);
		computeContext.TransitionResource(m_clothBuffer[1], ResourceState::ShaderResource);

		computeContext.SetRootSignature(m_computeRootSignature);
		computeContext.SetComputePipeline(m_computePipeline);

		const uint32_t iterations = 64;
		for (uint32_t j = 0; j < iterations; ++j)
		{
			m_readSet = 1 - m_readSet;
			if (j == iterations - 1)
				computeContext.SetResources(m_computeNormalResources);
			else
				computeContext.SetResources(m_computeResources[m_readSet]);
			computeContext.Dispatch2D(m_gridSize[0], m_gridSize[1]);

			if (j != iterations - 1)
			{
				computeContext.InsertUAVBarrier(m_clothBuffer[0]);
				computeContext.InsertUAVBarrier(m_clothBuffer[1]);
			}
		}
	}

	auto colorBuffer = GetColorBuffer();
	context.TransitionResource(colorBuffer, ResourceState::RenderTarget);
	context.TransitionResource(m_depthBuffer, ResourceState::DepthWrite);
	context.TransitionResource(m_clothBuffer[0], ResourceState::VertexBuffer);
	Color clearColor{ DirectX::Colors::LightGray };
	context.ClearColor(colorBuffer, clearColor);
	context.ClearDepth(m_depthBuffer);

	context.BeginRendering(colorBuffer, m_depthBuffer);

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());

	// Sphere
	context.SetRootSignature(m_sphereRootSignature);
	context.SetGraphicsPipeline(m_spherePipeline);

	context.SetResources(m_sphereResources);

	m_sphereModel->Render(context);

	// Cloth
	context.SetRootSignature(m_clothRootSignature);
	context.SetGraphicsPipeline(m_clothPipeline);

	context.SetResources(m_clothResources);

	context.SetIndexBuffer(m_clothIndexBuffer);
	context.SetVertexBuffer(0, m_clothBuffer[0]);

	context.DrawIndexed((uint32_t)m_clothIndexBuffer->GetElementCount());

	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(colorBuffer, ResourceState::Present);

	context.Finish();
}


void ComputeClothApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		512.0f);
	m_camera.SetPosition(Math::Vector3(-2.0f, -2.0f, -2.0f));

	m_controller.SetSpeedScale(0.01f);
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.SetOrbitTarget(Vector3(0.0f, 0.0f, 0.0f), Length(m_camera.GetPosition()), 2.0f);

	InitRootSignatures();
	InitConstantBuffers();
	InitCloth();

	LoadAssets();

	InitResourceSets();
}


void ComputeClothApp::CreateWindowSizeDependentResources()
{
	InitDepthBuffer();
	if (!m_pipelinesCreated)
	{
		InitPipelines();
		m_pipelinesCreated = true;
	}

	// Update the camera since the aspect ratio might have changed
	m_camera.SetPerspectiveMatrix(
		XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		512.0f);
	m_camera.Update();
}


void ComputeClothApp::InitDepthBuffer()
{
	DepthBufferDesc depthBufferDesc{
		.name		= "Depth Buffer",
		.width		= GetWindowWidth(),
		.height		= GetWindowHeight(),
		.format		= GetDepthFormat()
	};

	m_depthBuffer = CreateDepthBuffer(depthBufferDesc);
}


void ComputeClothApp::InitRootSignatures()
{
	// Sphere graphics
	RootSignatureDesc sphereRootSignatureDesc{
		.name				= "Sphere Root Signature",
		.flags				= RootSignatureFlags::AllowInputAssemblerInputLayout | RootSignatureFlags::DenyPixelShaderRootAccess,
		.rootParameters		= {	RootParameter::RootCBV(0, ShaderStage::Vertex),	}
	};
	m_sphereRootSignature = CreateRootSignature(sphereRootSignatureDesc);

	// Cloth graphics
	RootSignatureDesc clothRootSignatureDesc{
		.name				= "Cloth Root Signature",
		.flags				= RootSignatureFlags::AllowInputAssemblerInputLayout,
		.rootParameters		= {	
			RootParameter::RootCBV(0, ShaderStage::Vertex),	
			RootParameter::Range(DescriptorType::TextureSRV, 0, 1, ShaderStage::Pixel),
			RootParameter::Range(DescriptorType::Sampler, 0, 1, ShaderStage::Pixel)
		}
	};
	m_clothRootSignature = CreateRootSignature(clothRootSignatureDesc);

	// Cloth sim compute
	RootSignatureDesc clothSimRootSignatureDesc{
		.name = "Cloth Sim Root Signature",
		.rootParameters = {
			RootParameter::Table( 
				{ DescriptorRange::StructuredBufferSRV(0), DescriptorRange::StructuredBufferUAV(1), DescriptorRange::ConstantBuffer(2) },
				ShaderStage::Compute),
		}
	};
	m_computeRootSignature = CreateRootSignature(clothSimRootSignatureDesc);
}


void ComputeClothApp::InitPipelines()
{
	// Sphere - graphics
	{
		auto vertexLayout = VertexLayout<VertexComponent::PositionNormal>();

		VertexStreamDesc vertexStreamDesc{
			.inputSlot				= 0,
			.stride					= vertexLayout.GetSizeInBytes(),
			.inputClassification	= InputClassification::PerVertexData
		};

		GraphicsPipelineDesc spherePipelineDesc{
			.name				= "Sphere Graphics PSO",
			.blendState			= CommonStates::BlendDisable(),
			.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
			.rasterizerState	= CommonStates::RasterizerDefault(),
			.rtvFormats			= { GetColorFormat() },
			.dsvFormat			= GetDepthFormat(),
			.topology			= PrimitiveTopology::TriangleList,
			.vertexShader		= {.shaderFile = "SphereVS" },
			.pixelShader		= {.shaderFile = "SpherePS" },
			.vertexStreams		= { vertexStreamDesc },
			.vertexElements		= vertexLayout.GetElements(),
			.rootSignature		= m_sphereRootSignature
		};
		m_spherePipeline = CreateGraphicsPipeline(spherePipelineDesc);
	}

	// Cloth - graphics
	{
		vector<VertexElementDesc> clothVtxElements =
		{
			{ "POSITION", 0, Format::RGB32_Float, 0, offsetof(Particle, pos), InputClassification::PerVertexData, 0 },
			{ "TEXCOORD", 0, Format::RG32_Float, 0, offsetof(Particle, uv), InputClassification::PerVertexData, 0 },
			{ "NORMAL", 0, Format::RGB32_Float, 0, offsetof(Particle, normal), InputClassification::PerVertexData, 0 },

		};

		VertexStreamDesc vertexStreamDesc{
			.inputSlot				= 0,
			.stride					= sizeof(Particle),
			.inputClassification	= InputClassification::PerVertexData
		};

		GraphicsPipelineDesc clothPipelineDesc{
			.name					= "Cloth Graphics PSO",
			.blendState				= CommonStates::BlendDisable(),
			.depthStencilState		= CommonStates::DepthStateReadWriteReversed(),
			.rasterizerState		= CommonStates::RasterizerDefault(),
			.rtvFormats				= { GetColorFormat() },
			.dsvFormat				= GetDepthFormat(),
			.topology				= PrimitiveTopology::TriangleStrip,
			.indexBufferStripCut	= IndexBufferStripCutValue::Value_0xFFFFFFFF,
			.vertexShader			= {.shaderFile = "ClothVS" },
			.pixelShader			= {.shaderFile = "ClothPS" },
			.vertexStreams			= { vertexStreamDesc },
			.vertexElements			= clothVtxElements,
			.rootSignature			= m_clothRootSignature
		};
		m_clothPipeline = CreateGraphicsPipeline(clothPipelineDesc);
	}

	// Cloth sim - compute
	{
		ComputePipelineDesc clothSimPipelineDesc{
			.name			= "Cloth Sim Compute PSO",
			.computeShader	= {.shaderFile = "ClothCS"},
			.rootSignature	= m_computeRootSignature
		};
		m_computePipeline = CreateComputePipeline(clothSimPipelineDesc);
	}
}


void ComputeClothApp::InitConstantBuffers()
{
	// VS constants
	m_vsConstants.lightPos = Vector4(2.0f, 4.0f, 2.0f, 1.0f);

	GpuBufferDesc vsConstantBufferDesc{
		.name			= "VS Constant Buffer",
		.resourceType	= ResourceType::ConstantBuffer,
		.memoryAccess	= MemoryAccess::GpuRead | MemoryAccess::CpuWrite,
		.elementCount	= 1,
		.elementSize	= sizeof(VSConstants),
		.initialData	= &m_vsConstants
	};
	m_vsConstantBuffer = CreateGpuBuffer(vsConstantBufferDesc);

	// CS constants
	const float dx = m_size[0] / (m_gridSize[0] - 1);
	const float dy = m_size[1] / (m_gridSize[1] - 1);

	m_csConstants.deltaT = 0.0f;
	m_csConstants.particleMass = 0.1f;
	m_csConstants.springStiffness = 2000.0f;
	m_csConstants.damping = 0.25f;
	m_csConstants.restDistH = dx;
	m_csConstants.restDistV = dy;
	m_csConstants.restDistD = sqrtf(dx * dx + dy * dy);
	m_csConstants.sphereRadius = m_sphereRadius;
	m_csConstants.spherePos = m_pinnedCloth ? Vector4(0.0f, 0.0f, -10.0f, 0.0f) : Vector4(0.0f);
	m_csConstants.gravity = Vector4(0.0f, -9.8f, 0.0f, 0.0f);
	m_csConstants.particleCount[0] = m_gridSize[0];
	m_csConstants.particleCount[1] = m_gridSize[1];
	m_csConstants.calculateNormals = 0;

	GpuBufferDesc csConstantBufferDesc{
		.name			= "CS Constant Buffer",
		.resourceType	= ResourceType::ConstantBuffer,
		.memoryAccess	= MemoryAccess::GpuRead | MemoryAccess::CpuWrite,
		.elementCount	= 1,
		.elementSize	= sizeof(CSConstants),
		.initialData	= &m_csConstants
	};
	m_csConstantBuffer = CreateGpuBuffer(csConstantBufferDesc);

	// CS normal constant buffer
	m_csConstants.calculateNormals = 1;
	GpuBufferDesc csNormalConstantBufferDesc = csConstantBufferDesc;
	csNormalConstantBufferDesc.SetName("CS Normal Constant Buffer");
	m_csNormalConstantBuffer = CreateGpuBuffer(csNormalConstantBufferDesc);
}


void ComputeClothApp::InitCloth()
{
	const uint32_t numParticles = m_gridSize[0] * m_gridSize[1];
	vector<Particle> particles(m_gridSize[0] * m_gridSize[1]);

	float dx = m_size[0] / (m_gridSize[0] - 1);
	float dy = m_size[1] / (m_gridSize[1] - 1);
	float du = 1.0f / (m_gridSize[0] - 1);
	float dv = 1.0f / (m_gridSize[1] - 1);

	if (m_pinnedCloth)
	{
		Matrix4 transMat = AffineTransform::MakeTranslation(Vector3(-m_size[0] / 2.0f, -m_size[1] / 2.0f, 0.0f));
		for (uint32_t i = 0; i < m_gridSize[1]; i++)
		{
			for (uint32_t j = 0; j < m_gridSize[0]; j++)
			{
				auto& particle = particles[i + j * m_gridSize[1]];
				particle.pos = transMat * Vector4(dx * j, dy * i, 0.0f, 1.0f);
				particle.vel = Vector4(0.0f);
				particle.uv = Vector4(du * j, dv * i, 0.0f, 0.0f);
				particle.normal = Vector4(0.0f, 1.0f, 0.0f, 0.0f);
				float p = (i == 0) && ((j == 0) || (j == m_gridSize[0] / 3) || (j == m_gridSize[0] - m_gridSize[0] / 3) || (j == m_gridSize[0] - 1));
				particle.pinned = Vector4(p, 0.0f, 0.0f, 0.0f);
			}
		}
	}
	else
	{
		Matrix4 transMat = AffineTransform::MakeTranslation(Vector3(-m_size[0] / 2.0f, 2.0f, -m_size[1] / 2.0f));
		for (uint32_t i = 0; i < m_gridSize[1]; i++)
		{
			for (uint32_t j = 0; j < m_gridSize[0]; j++)
			{
				auto& particle = particles[i + j * m_gridSize[1]];
				particle.pos = transMat * Vector4(dx * j, 0.0f, dy * i, 1.0f);
				particle.vel = Vector4(0.0f);
				particle.uv = Vector4(1.0f - du * i, dv * j, 0.0f, 0.0f);
				particle.normal = Vector4(0.0f, 1.0f, 0.0f, 0.0f);
				particle.pinned = Vector4(0.0f, 0.0f, 0.0f, 0.0f);
			}
		}
	}

	// Vertices
	GpuBufferDesc clothVertexBufferDesc{
		.name					= "Cloth Vertex Buffer 0",
		.resourceType			= ResourceType::VertexBuffer,
		.memoryAccess			= MemoryAccess::GpuReadWrite,
		.elementCount			= numParticles,
		.elementSize			= sizeof(Particle),
		.initialData			= particles.data(),
		.bAllowShaderResource	= true,
		.bAllowUnorderedAccess	= true
	};
	m_clothBuffer[0] = CreateGpuBuffer(clothVertexBufferDesc);

	clothVertexBufferDesc.SetName("Cloth Vertex Buffer 1");
	m_clothBuffer[1] = CreateGpuBuffer(clothVertexBufferDesc);

	// Indices
	vector<uint32_t> indices;
	for (uint32_t y = 0; y < m_gridSize[1] - 1; y++)
	{
		for (uint32_t x = 0; x < m_gridSize[0]; x++)
		{
			indices.push_back((y + 1) * m_gridSize[0] + x);
			indices.push_back((y)*m_gridSize[0] + x);
		}
		// Primitive restart (signaled by special value 0xFFFFFFFF)
		indices.push_back(0xFFFFFFFF);
	}

	// Cloth index buffer
	GpuBufferDesc clothIndexBufferDesc{
		.name			= "Cloth Index Buffer",
		.resourceType	= ResourceType::IndexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= indices.size(),
		.elementSize	= sizeof(uint32_t),
		.initialData	= indices.data()
	};
	m_clothIndexBuffer = CreateGpuBuffer(clothIndexBufferDesc);
}


void ComputeClothApp::InitResourceSets()
{
	m_sphereResources.Initialize(m_sphereRootSignature);
	m_sphereResources.SetCBV(0, 0, m_vsConstantBuffer);

	m_clothResources.Initialize(m_clothRootSignature);
	m_clothResources.SetCBV(0, 0, m_vsConstantBuffer);
	m_clothResources.SetSRV(1, 0, m_texture);
	m_clothResources.SetSampler(2, 0, m_sampler);

	m_computeResources[0].Initialize(m_computeRootSignature);
	m_computeResources[0].SetSRV(0, 0, m_clothBuffer[0]);
	m_computeResources[0].SetUAV(0, 1, m_clothBuffer[1]);
	m_computeResources[0].SetCBV(0, 2, m_csConstantBuffer);

	m_computeResources[1].Initialize(m_computeRootSignature);
	m_computeResources[1].SetSRV(0, 0, m_clothBuffer[1]);
	m_computeResources[1].SetUAV(0, 1, m_clothBuffer[0]);
	m_computeResources[1].SetCBV(0, 2, m_csConstantBuffer);

	m_computeNormalResources.Initialize(m_computeRootSignature);
	m_computeNormalResources.SetSRV(0, 0, m_clothBuffer[1]);
	m_computeNormalResources.SetUAV(0, 1, m_clothBuffer[0]);
	m_computeNormalResources.SetCBV(0, 2, m_csNormalConstantBuffer);
}


void ComputeClothApp::LoadAssets()
{
	auto layout = VertexLayout<VertexComponent::PositionNormal>();
	m_sphereModel = LoadModel("sphere.gltf", layout, m_sphereRadius * 1.0f);

	if (m_appInfo.api == GraphicsApi::D3D12)
	{
		m_texture = LoadTexture("DirectXLogo.dds");
	}
	else
	{
		m_texture = LoadTexture("vulkan_cloth_rgba.ktx2");
	}
	m_sampler = CreateSampler(CommonStates::SamplerLinearClamp());
}


void ComputeClothApp::UpdateConstantBuffers()
{
	m_vsConstants.projectionMatrix = m_camera.GetProjMatrix();
	m_vsConstants.modelViewMatrix = m_camera.GetViewMatrix();
	m_vsConstantBuffer->Update(sizeof(VSConstants), &m_vsConstants);

	m_csConstants.deltaT = (float)m_timer.GetElapsedSeconds() / 64.0f;
	if (m_simulateWind)
	{
		m_csConstants.gravity.SetX(cosf(XMConvertToRadians(-(float)m_timer.GetTotalSeconds() * 360.0f)) * (g_rng.NextFloat(1.0f, 6.0f) - g_rng.NextFloat(1.0f, 6.0f)));
		m_csConstants.gravity.SetZ(sinf(XMConvertToRadians((float)m_timer.GetTotalSeconds() * 360.0f)) * (g_rng.NextFloat(1.0f, 6.0f) - g_rng.NextFloat(1.0f, 6.0f)));
	}
	else
	{
		m_csConstants.gravity.SetX(0.0f);
		m_csConstants.gravity.SetZ(0.0f);
	}

	m_csConstants.calculateNormals = 0;
	m_csConstantBuffer->Update(sizeof(CSConstants), &m_csConstants);

	m_csConstants.calculateNormals = 1;
	m_csNormalConstantBuffer->Update(sizeof(CSConstants), &m_csConstants);
}