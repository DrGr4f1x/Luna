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

#include "ComputeNBodyApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"

using namespace Luna;
using namespace Math;
using namespace std;

#define PARTICLES_PER_ATTRACTOR 4 * 1024


ComputeNBodyApp::ComputeNBodyApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
	, m_controller{ m_camera, Vector3(kYUnitVector) }
{}


int ComputeNBodyApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void ComputeNBodyApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void ComputeNBodyApp::Startup()
{
	// Application initialization, after device creation
}


void ComputeNBodyApp::Shutdown()
{
	// Application cleanup on shutdown
}


void ComputeNBodyApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffers();
}


void ComputeNBodyApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	// Particle simulation
	{
		auto& computeContext = context.GetComputeContext();

		computeContext.TransitionResource(m_particleBuffer, ResourceState::UnorderedAccess);

		computeContext.SetRootSignature(m_computeRootSignature);
		computeContext.SetComputePipeline(m_computeCalculatePipeline);

		computeContext.SetDescriptors(0, m_computeCbvUavDescriptorSet);

		computeContext.Dispatch1D(6 * PARTICLES_PER_ATTRACTOR, 256);

		computeContext.InsertUAVBarrier(m_particleBuffer);

		computeContext.SetComputePipeline(m_computeIntegratePipeline);

		computeContext.Dispatch1D(6 * PARTICLES_PER_ATTRACTOR, 256);

		computeContext.TransitionResource(m_particleBuffer, ResourceState::NonPixelShaderResource);
	}

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(GetDepthBuffer(), ResourceState::DepthWrite);
	context.ClearColor(GetColorBuffer());
	context.ClearDepth(GetDepthBuffer());

	context.BeginRendering(GetColorBuffer(), GetDepthBuffer());

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());

	// Draw particles
	context.SetRootSignature(m_rootSignature);
	context.SetGraphicsPipeline(m_graphicsPipeline);

	context.SetDescriptors(0, m_graphicsVsCbvSrvDescriptorSet);
	context.SetDescriptors(1, m_graphicsGsCbvDescriptorSet);
	context.SetDescriptors(2, m_graphicsPsSrvDescriptorSet);
	context.SetDescriptors(3, m_samplerDescriptorSet);

	context.Draw(6 * PARTICLES_PER_ATTRACTOR);

	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void ComputeNBodyApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		512.0f);
	m_camera.SetPosition(Math::Vector3(8.0f, 8.0f, 8.0f));

	m_controller.SetSpeedScale(0.01f);
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.SetOrbitTarget(Vector3(0.0f, 0.0f, 0.0f), Length(m_camera.GetPosition()), 2.0f);

	InitRootSignatures();
	
	// Create constant buffers
	m_graphicsConstantBuffer = CreateConstantBuffer("Graphics Constant Buffer", 1, sizeof(GraphicsConstants));
	m_computeConstantBuffer = CreateConstantBuffer("Compute Constant Buffer", 1, sizeof(ComputeConstants));

	InitParticles();

	LoadAssets();

	InitDescriptorSets();
}


void ComputeNBodyApp::CreateWindowSizeDependentResources()
{
	if (!m_pipelinesCreated)
	{
		InitPipelines();
		m_pipelinesCreated = true;
	}

	m_camera.SetPerspectiveMatrix(
		XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		512.0f);
}


void ComputeNBodyApp::InitRootSignatures()
{
	RootSignatureDesc graphicsRootSignatureDesc{
		.name				= "Graphics Root Signature",
		.flags				= RootSignatureFlags::AllowInputAssemblerInputLayout,
		.rootParameters		= {
			Table({ ConstantBuffer, StructuredBufferSRV }, ShaderStage::Vertex),
			Table({ ConstantBuffer }, ShaderStage::Geometry),
			Table({ TextureSRV(0, 2) }, ShaderStage::Pixel),
			Table({ Sampler }, ShaderStage::Pixel)
		}
	};
	m_rootSignature = CreateRootSignature(graphicsRootSignatureDesc);

	RootSignatureDesc computeRootSignatureDesc{
		.name				= "Compute Root Signature",
		.rootParameters		= {
			Table({ ConstantBuffer, StructuredBufferUAV(1) }, ShaderStage::Compute)
		}
	};
	m_computeRootSignature = CreateRootSignature(computeRootSignatureDesc);
}


void ComputeNBodyApp::InitPipelines()
{
	// Graphics pipeline
	BlendStateDesc blendStateDesc{};
	blendStateDesc.alphaToCoverageEnable = false;
	blendStateDesc.independentBlendEnable = false;
	blendStateDesc.renderTargetBlend[0].blendEnable = true;
	blendStateDesc.renderTargetBlend[0].srcBlend = Blend::One;
	blendStateDesc.renderTargetBlend[0].dstBlend = Blend::One;
	blendStateDesc.renderTargetBlend[0].blendOp = BlendOp::Add;
	blendStateDesc.renderTargetBlend[0].srcBlendAlpha = Blend::SrcAlpha;
	blendStateDesc.renderTargetBlend[0].dstBlendAlpha = Blend::DstAlpha;
	blendStateDesc.renderTargetBlend[0].blendOpAlpha = BlendOp::Add;
	blendStateDesc.renderTargetBlend[0].writeMask = ColorWrite::All;

	GraphicsPipelineDesc graphicsPipelineDesc{
		.name				= "Graphics PSO",
		.blendState			= blendStateDesc,
		.depthStencilState	= CommonStates::DepthStateDisabled(),
		.rasterizerState	= CommonStates::RasterizerTwoSided(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::PointList,
		.vertexShader		= {.shaderFile = "ParticleVS" },
		.pixelShader		= {.shaderFile = "ParticlePS" },
		.geometryShader		= {.shaderFile = "ParticleGS" },
		.rootSignature		= m_rootSignature
	};

	m_graphicsPipeline = CreateGraphicsPipeline(graphicsPipelineDesc);

	// Compute pipelines
	ComputePipelineDesc computeCalculatePipelineDesc{
		.name			= "Compute Calculate PSO",
		.computeShader	= { .shaderFile = "ParticleCalculateCS" },
		.rootSignature	= m_computeRootSignature
	};
	m_computeCalculatePipeline = CreateComputePipeline(computeCalculatePipelineDesc);

	ComputePipelineDesc computeIntegratePipelineDesc{
		.name			= "Compute Integrate PSO",
		.computeShader	= {.shaderFile = "ParticleIntegrateCS" },
		.rootSignature	= m_computeRootSignature
	};
	m_computeIntegratePipeline = CreateComputePipeline(computeIntegratePipelineDesc);
}


void ComputeNBodyApp::InitDescriptorSets()
{
	m_graphicsVsCbvSrvDescriptorSet = m_rootSignature->CreateDescriptorSet(0);
	m_graphicsVsCbvSrvDescriptorSet->SetCBV(0, m_graphicsConstantBuffer);
	m_graphicsVsCbvSrvDescriptorSet->SetSRV(1, m_particleBuffer);

	m_graphicsGsCbvDescriptorSet = m_rootSignature->CreateDescriptorSet(1);
	m_graphicsGsCbvDescriptorSet->SetCBV(0, m_graphicsConstantBuffer);

	m_graphicsPsSrvDescriptorSet = m_rootSignature->CreateDescriptorSet(2);
	m_graphicsPsSrvDescriptorSet->SetSRV(0, m_colorTexture);
	m_graphicsPsSrvDescriptorSet->SetSRV(1, m_gradientTexture);

	m_samplerDescriptorSet = m_rootSignature->CreateDescriptorSet(3);
	m_samplerDescriptorSet->SetSampler(0, m_sampler);

	m_computeCbvUavDescriptorSet = m_computeRootSignature->CreateDescriptorSet(0);
	m_computeCbvUavDescriptorSet->SetCBV(0, m_computeConstantBuffer);
	m_computeCbvUavDescriptorSet->SetUAV(1, m_particleBuffer);
}


void ComputeNBodyApp::InitParticles()
{
	vector<Vector3> attractors =
	{
		Vector3(5.0f, 0.0f, 0.0f),
		Vector3(-5.0f, 0.0f, 0.0f),
		Vector3(0.0f, 0.0f, 5.0f),
		Vector3(0.0f, 0.0f, -5.0f),
		Vector3(0.0f, 4.0f, 0.0f),
		Vector3(0.0f, -8.0f, 0.0f),
	};

	uint32_t numParticles = static_cast<uint32_t>(attractors.size()) * PARTICLES_PER_ATTRACTOR;

	vector<Particle> particles(numParticles);

	for (uint32_t i = 0; i < static_cast<uint32_t>(attractors.size()); i++)
	{
		for (uint32_t j = 0; j < PARTICLES_PER_ATTRACTOR; j++)
		{
			Particle& particle = particles[i * PARTICLES_PER_ATTRACTOR + j];

			// First particle in group as heavy center of gravity
			if (j == 0)
			{
				particle.pos = Vector4(1.5f * attractors[i], 90000.0f);
				particle.vel = Vector4(0.0f);
			}
			else
			{
				// Position					
				Vector3 position(attractors[i] + 0.75f * Vector3(g_rng.Normal(), g_rng.Normal(), g_rng.Normal()));
				float len = Length(Normalize(position - attractors[i]));
				position.SetY(position.GetY() * (2.0f - (len * len)));

				// Velocity
				Vector3 angular = (((i % 2) == 0) ? 1.0f : -1.0f) * Vector3(0.5f, 1.5f, 0.5f);
				Vector3 velocity = Cross((position - attractors[i]), angular) + Vector3(g_rng.Normal(), g_rng.Normal(), g_rng.Normal() * 0.025f);

				float mass = (g_rng.Normal() * 0.5f + 0.5f) * 75.0f;
				particle.pos = Vector4(position, mass);
				particle.vel = Vector4(velocity, 0.0f);
			}

			// Color gradient offset
			particle.vel.SetW((float)i * 1.0f / static_cast<uint32_t>(attractors.size()));
		}
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


void ComputeNBodyApp::LoadAssets()
{
	m_gradientTexture = LoadTexture("particle_gradient_rgba.ktx");
	m_colorTexture = LoadTexture("particle01_rgba.ktx");
	m_sampler = CreateSampler(CommonStates::SamplerLinearClamp());
}


void ComputeNBodyApp::UpdateConstantBuffers()
{
	m_graphicsConstants.projectionMatrix = m_camera.GetProjectionMatrix();
	m_graphicsConstants.modelViewMatrix = m_camera.GetViewMatrix();
	m_graphicsConstants.invViewMatrix = Invert(m_camera.GetViewMatrix());
	m_graphicsConstants.screenDim[0] = (float)GetWindowWidth();
	m_graphicsConstants.screenDim[1] = (float)GetWindowHeight();

	m_graphicsConstantBuffer->Update(sizeof(GraphicsConstants), &m_graphicsConstants);

	m_computeConstants.deltaT = (float)m_timer.GetElapsedSeconds() * 0.05f;
	m_computeConstants.destX = sinf(XMConvertToRadians((float)m_timer.GetElapsedSeconds() * 360.0f)) * 0.75f;
	m_computeConstants.destY = 0.0f;
	m_computeConstants.particleCount = 6 * PARTICLES_PER_ATTRACTOR;

	m_computeConstantBuffer->Update(sizeof(ComputeConstants), &m_computeConstants);
}