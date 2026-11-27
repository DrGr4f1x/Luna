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

#include "RayTracingCallableApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"
#include "Graphics\Device.h"

using namespace Luna;
using namespace Math;
using namespace std;


RayTracingCallableApp::RayTracingCallableApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{
}


int RayTracingCallableApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void RayTracingCallableApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void RayTracingCallableApp::Startup()
{
	// Application initialization, after device creation
}


void RayTracingCallableApp::Shutdown()
{
	// Application cleanup on shutdown
}


void RayTracingCallableApp::Update()
{
	// Application update tick
	UpdateConstantBuffer();
}


void RayTracingCallableApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	// Ray trace scene
	context.TransitionResource(m_colorTarget, ResourceState::UnorderedAccess);

	context.BindDescriptorHeap(GetDefaultDescriptorHeap());

	auto& computeContext = context.GetComputeContext();

	computeContext.SetRootSignature(m_rootSignature);
	computeContext.SetRayTracingPipeline(m_rayTracingPipeline);
	computeContext.SetRootSRV(0, m_tlasBuffer);
	computeContext.SetDescriptors(1, m_descriptorSet);

	computeContext.DispatchRays(m_shaderBindingTable, GetWindowWidth(), GetWindowHeight());

	// Copy ray tracing results to back buffer
	context.CopyColorBuffer(GetColorBuffer(), m_colorTarget);

	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void RayTracingCallableApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		256.0f);
	m_camera.SetPosition(Vector3(0.0f, 0.0f, 10.0f));

	m_sceneConstantBuffer = CreateConstantBuffer("Scene Constant Buffer", 1, sizeof(SceneConstants));

	InitRayTracingBLAS();
	InitRayTracingTLAS();
	InitRootSignature();
	InitRayTracingPipeline();
	InitShaderBindingTable();
}


void RayTracingCallableApp::CreateWindowSizeDependentResources()
{
	// Create any resources that depend on window size.  May be called when the window size changes.
	InitColorTarget();
	InitDescriptorSet();
	m_camera.SetAspectRatio(GetWindowAspectRatio());
}


void RayTracingCallableApp::InitRayTracingBLAS()
{
	// Setup triangle vertices
	struct Vertex
	{
		float pos[3];
	};
	vector<Vertex> vertices = {
		{ {  0.0f,   1.0f,  0.0f } },
		{ { -1.0f,  -1.0f,  0.0f } },
		{ {  1.0f,  -1.0f,  0.0f } }
	};
	auto vertexBuffer = CreateRayTracingVertexBuffer<Vertex>("Vertex Buffer", vertices);

	// Setup triangle indices
	vector<uint32_t> indices = { 0, 1, 2 };
	auto indexBuffer = CreateRayTracingIndexBuffer("Index Buffer", indices);

	// Setup transform matrices
	vector<Matrix4> transforms;
	for (uint32_t i = 0; i < 3; ++i)
	{
		Matrix4 transform = Matrix4::MakeTranslation((float)i * 3.0f - 3.0f, 0.0f, 0.0f);
		transforms.push_back(transform);
	}
	auto transformBuffer = m_device->CreateTransformBuffer(transforms);

	// Setup geometry descriptions
	vector<RayTracingGeometryDesc> geometries;
	for (uint32_t i = 0; i < 3; ++i)
	{
		RayTracingGeometryDesc blasDesc{
			.name			= "Triangle BLAS",
			.geometryType	= RayTracingGeometryType::Triangles,
			.flags			= RayTracingGeometryFlags::Opaque
		};
		blasDesc.triangles.transform3x4 = transformBuffer->GetGpuAddress();
		blasDesc.triangles.indexFormat = Format::R32_UInt;
		blasDesc.triangles.vertexFormat = Format::RGB32_Float;
		blasDesc.triangles.indexCount = indices.size();
		blasDesc.triangles.vertexCount = vertices.size();
		blasDesc.triangles.vertexStride = sizeof(Vertex);
		blasDesc.triangles.indexBuffer = indexBuffer->GetGpuAddress();
		blasDesc.triangles.vertexBuffer = vertexBuffer->GetGpuAddress();
		blasDesc.triangles.transformOffset = i;

		geometries.push_back(blasDesc);
	}

	m_blasBuffer = m_device->CreateBLAS(geometries, RayTracingAccelerationStructureFlags::FastTrace);
}


void RayTracingCallableApp::InitRayTracingTLAS()
{
	RayTracingInstanceDesc instanceDesc{
		.blas			= m_blasBuffer,
		.transform		= Matrix4{ kIdentity },
		.instanceID		= 1,
		.instanceMask	= 0xFF
	};

	vector<RayTracingInstanceDesc> instances = { instanceDesc };

	RayTracingTLASDesc tlasDesc{
		.instances = instances,
		.buildFlags = RayTracingAccelerationStructureFlags::FastTrace
	};

	m_tlasBuffer = GetDevice()->CreateTLAS(tlasDesc);
}


void RayTracingCallableApp::InitRootSignature()
{
	RootSignatureDesc desc{
		.name				= "Root Signature",
		.rootParameters		= {
			RootSRV(0, ShaderStage::RayGeneration, 0, true /* isAccelerationStructure */),
			Table({ TextureUAV, ConstantBuffer }, ShaderStage::RayGeneration)
		}
	};

	m_rootSignature = CreateRootSignature(desc);
}


void RayTracingCallableApp::InitRayTracingPipeline()
{
	RayTracingPipelineDesc desc{
		.rootSignature = m_rootSignature,
		.rayGenShaders = {
			{ ShaderType::RayGeneration, {.shaderFile = "RayGen", .entry = "raygen_main" }}
		},
		.missShaders = {
			{ ShaderType::Miss, {.shaderFile = "Miss", .entry = "miss_main" }},
		},
		.callableShaders = {
			{ ShaderType::Callable, { .shaderFile = "Callable1", .entry = "callable1_main" } },
			{ ShaderType::Callable, { .shaderFile = "Callable2", .entry = "callable2_main" } },
			{ ShaderType::Callable, { .shaderFile = "Callable3", .entry = "callable3_main" } }
		},
		.hitGroupShaders = {
			{ ShaderType::ClosestHit, {.shaderFile = "ClosestHit", .entry = "closesthit_main" }}
		},
		.hitGroups = { ShaderGroupDesc(0) },  // Index 0 in the hitGroupShaders list, i.e. the 'ClosestHit' shader
		.maxPayloadSize = 3 * sizeof(float),
		.maxRayHitAttributeSize = 2 * sizeof(float)
	};

	m_rayTracingPipeline = GetDevice()->CreateRayTracingPipeline(desc);
}


void RayTracingCallableApp::InitShaderBindingTable()
{
	ShaderBindingTableDesc desc{
		.rayGenShaders		= { ShaderRecord{ .id = 0 } }, // refers to the RayGen shader at index 0 in the library
		.missShaders		= { ShaderRecord{ .id = 0 } }, // refers to the Miss shader at index 0 in the library
		.callableShaders	= {
			ShaderRecord{ .id = 0 },
			ShaderRecord{ .id = 1 },
			ShaderRecord{ .id = 2 }
		},
		.hitGroups			= { ShaderRecord{ .id = 0 } }  // refers to the HitGroup at index 0 in the library
	};

	m_shaderBindingTable = GetDevice()->CreateShaderBindingTable(desc, m_rayTracingPipeline);
}


void RayTracingCallableApp::InitColorTarget()
{
	ColorBufferDesc colorDesc{
		.name = "Ray Tracing Target",
		.width = GetWindowWidth(),
		.height = GetWindowHeight(),
		.format = GetColorFormat()
	};
	m_colorTarget = CreateColorBuffer(colorDesc);
}


void RayTracingCallableApp::InitDescriptorSet()
{
	m_descriptorSet = m_rootSignature->CreateDescriptorSet(1, { m_colorTarget, m_sceneConstantBuffer });
}


void RayTracingCallableApp::UpdateConstantBuffer()
{
	m_sceneConstants.projectionToWorld = Invert(m_camera.GetViewProjectionMatrix());
	m_sceneConstants.cameraPosition = m_camera.GetPosition();

	m_sceneConstantBuffer->Update(sizeof(SceneConstants), &m_sceneConstants);
}