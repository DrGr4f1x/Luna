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

#include "ComputeParticlesApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"

using namespace Luna;
using namespace Math;
using namespace std;


ComputeParticlesApp::ComputeParticlesApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{}


int ComputeParticlesApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void ComputeParticlesApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void ComputeParticlesApp::Startup()
{
	// Application initialization, after device creation
}


void ComputeParticlesApp::Shutdown()
{
	// Application cleanup on shutdown
}


void ComputeParticlesApp::Update()
{
	if (m_animate)
	{
		if (m_animStart > 0.0f)
		{
			m_animStart -= (float)m_timer.GetElapsedSeconds() * 5.0f;
		}
		else if (m_animStart <= 0.0f)
		{
			m_localTimer += (float)m_timer.GetElapsedSeconds() * 0.04f;
			if (m_localTimer > 1.0f)
			{
				m_localTimer = 0.f;
			}
		}
	}

	UpdateConstantBuffers();
}


void ComputeParticlesApp::UpdateUI()
{
	if (m_uiOverlay->Header("Settings"))
	{
		m_uiOverlay->CheckBox("Moving attractor", &m_animate);
	}
}


void ComputeParticlesApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	// Simulate particles on compute
	{
		auto& computeContext = context.GetComputeContext();

		computeContext.TransitionResource(m_particleBuffer, ResourceState::UnorderedAccess);

		computeContext.SetRootSignature(m_computeRootSignature);
		computeContext.SetComputePipeline(m_computePipeline);

		computeContext.SetResources(m_computeResources);

		computeContext.Dispatch1D(m_particleCount, 256);

		computeContext.TransitionResource(m_particleBuffer, ResourceState::NonPixelShaderResource);
	}

	// Render particles
	context.TransitionResource(m_particleBuffer, ResourceState::UnorderedAccess);
	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(GetDepthBuffer(), ResourceState::DepthWrite);
	context.ClearColor(GetColorBuffer());
	context.ClearDepth(GetDepthBuffer());

	context.BeginRendering(GetColorBuffer(), GetDepthBuffer());

	context.SetRootSignature(m_graphicsRootSignature);
	context.SetGraphicsPipeline(m_graphicsPipeline);

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());

	context.SetResources(m_gfxResources);

	context.Draw(6 * m_particleCount);

	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void ComputeParticlesApp::CreateDeviceDependentResources()
{
	InitRootSignatures();
	InitConstantBuffers();
	InitParticles();

	LoadAssets();

	InitResourceSets();
}


void ComputeParticlesApp::CreateWindowSizeDependentResources()
{
	if (!m_pipelinesCreated)
	{
		InitPipelines();
		m_pipelinesCreated = true;
	}
}


void ComputeParticlesApp::InitRootSignatures()
{
	RootSignatureDesc computeRootSignatureDesc{
		.name				= "Compute Root Signature",
		.rootParameters		= {	
			Table({ StructuredBufferUAV, ConstantBuffer },  ShaderStage::Compute)
		}
	};

	m_computeRootSignature = CreateRootSignature(computeRootSignatureDesc);

	RootSignatureDesc graphicsRootSignatureDesc{
		.name				= "Graphics Root Signature",
		.rootParameters		= {	
			Table({ StructuredBufferSRV, ConstantBuffer },  ShaderStage::Vertex),
			Table({ TextureSRV(0, 2) }, ShaderStage::Pixel),
			Table({ Sampler }, ShaderStage::Pixel)
		}
	};

	m_graphicsRootSignature = CreateRootSignature(graphicsRootSignatureDesc);
}


void ComputeParticlesApp::InitPipelines()
{
	// Graphics pipeline
	BlendStateDesc blendStateDesc{};
	blendStateDesc.alphaToCoverageEnable = false;
	blendStateDesc.independentBlendEnable = false;
	blendStateDesc.renderTargetBlend[0].blendEnable = true;
	blendStateDesc.renderTargetBlend[0].srcBlend = Blend::One;
	blendStateDesc.renderTargetBlend[0].dstBlend = Blend::One;
	blendStateDesc.renderTargetBlend[0].blendOp = BlendOp::Add;
	blendStateDesc.renderTargetBlend[0].srcBlendAlpha = Blend::One;
	blendStateDesc.renderTargetBlend[0].dstBlendAlpha = Blend::One;
	blendStateDesc.renderTargetBlend[0].blendOpAlpha = BlendOp::Add;
	blendStateDesc.renderTargetBlend[0].writeMask = ColorWrite::All;

	GraphicsPipelineDesc graphicsPipelineDesc{
		.name				= "Graphics PSO",
		.blendState			= blendStateDesc,
		.depthStencilState	= CommonStates::DepthStateDisabled(),
		.rasterizerState	= CommonStates::RasterizerDefault(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "ParticlesVS" },
		.pixelShader		= { .shaderFile = "ParticlesPS" },
		.rootSignature		= m_graphicsRootSignature
	};

	m_graphicsPipeline = CreateGraphicsPipeline(graphicsPipelineDesc);

	// Compute pipeline
	ComputePipelineDesc computePipelineDesc{
		.name			= "Compute PSO",
		.computeShader	= {.shaderFile = "ParticlesCS" },
		.rootSignature	= m_computeRootSignature
	};
	m_computePipeline = CreateComputePipeline(computePipelineDesc);
}


void ComputeParticlesApp::InitConstantBuffers()
{
	// VS constant buffer
	GpuBufferDesc vsConstantBufferDesc{
		.name			= "VS Constant Buffer",
		.resourceType	= ResourceType::ConstantBuffer,
		.memoryAccess	= MemoryAccess::GpuRead | MemoryAccess::CpuWrite,
		.elementCount	= 1,
		.elementSize	= sizeof(VSConstants)
	};
	m_vsConstantBuffer = CreateGpuBuffer(vsConstantBufferDesc);

	// CS constant buffer
	GpuBufferDesc csConstantBufferDesc{
		.name			= "CS Constant Buffer",
		.resourceType	= ResourceType::ConstantBuffer,
		.memoryAccess	= MemoryAccess::GpuRead | MemoryAccess::CpuWrite,
		.elementCount	= 1,
		.elementSize	= sizeof(CSConstants)
	};
	m_csConstantBuffer = CreateGpuBuffer(csConstantBufferDesc);
}


void ComputeParticlesApp::InitParticles()
{
	vector<Particle> particles(m_particleCount);
	RandomNumberGenerator rng;

	for (auto& particle : particles)
	{
		particle.pos[0] = rng.NextFloat(-1.0f, 1.0f);
		particle.pos[1] = rng.NextFloat(-1.0f, 1.0f);
		particle.vel[0] = 0.0f;
		particle.vel[1] = 0.0f;
		particle.gradientPos = Vector4(0.5f * particle.pos[0], 0.0f, 0.0f, 0.0f);
	}

	GpuBufferDesc particleBufferDesc{
		.name			= "Particle Buffer",
		.resourceType	= ResourceType::StructuredBuffer,
		.memoryAccess	= MemoryAccess::GpuReadWrite,
		.elementCount	= particles.size(),
		.elementSize	= sizeof(Particle),
		.initialData	= particles.data()
	};
	m_particleBuffer = CreateGpuBuffer(particleBufferDesc);
}


void ComputeParticlesApp::InitResourceSets()
{
	m_computeResources.Initialize(m_computeRootSignature);
	m_computeResources.SetUAV(0, 0, m_particleBuffer);
	m_computeResources.SetCBV(0, 1, m_csConstantBuffer);

	m_gfxResources.Initialize(m_graphicsRootSignature);
	m_gfxResources.SetSRV(0, 0, m_particleBuffer);
	m_gfxResources.SetCBV(0, 1, m_vsConstantBuffer);
	m_gfxResources.SetSRV(1, 0, m_colorTexture);
	m_gfxResources.SetSRV(1, 1, m_gradientTexture);
	m_gfxResources.SetSampler(2, 0, m_sampler);
}


void ComputeParticlesApp::UpdateConstantBuffers()
{
	m_csConstants.deltaT = (float)m_timer.GetElapsedSeconds() * 2.5f;
	m_csConstants.destX = sinf(DirectX::XMConvertToRadians(m_localTimer * 360.0f)) * 0.75f;;
	m_csConstants.destY = 0.0f;
	m_csConstants.particleCount = m_particleCount;

	m_csConstantBuffer->Update(sizeof(CSConstants), &m_csConstants);

	m_vsConstants.invTargetSize[0] = 1.0f / GetWindowWidth();
	m_vsConstants.invTargetSize[1] = 1.0f / GetWindowHeight();
	m_vsConstants.pointSize = 8;

	m_vsConstantBuffer->Update(sizeof(VSConstants), &m_vsConstants);
}


void ComputeParticlesApp::LoadAssets()
{
	m_colorTexture = LoadTexture("particle01_rgba.ktx");
	m_gradientTexture = LoadTexture("particle_gradient_rgba.ktx");
	m_sampler = CreateSampler(CommonStates::SamplerLinearWrap());
}