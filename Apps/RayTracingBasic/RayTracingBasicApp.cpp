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

#include "RayTracingBasicApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"
#include "Graphics\Device.h"

#include "Graphics\RayTracing\RayTracingGeometry.h"

using namespace Luna;
using namespace Math;
using namespace std;


RayTracingBasicApp::RayTracingBasicApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{
}


int RayTracingBasicApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void RayTracingBasicApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void RayTracingBasicApp::Startup()
{
	// Application initialization, after device creation
}


void RayTracingBasicApp::Shutdown()
{
	// Application cleanup on shutdown
}


void RayTracingBasicApp::Update()
{
	// Application update tick
	UpdateConstantBuffer();
}


void RayTracingBasicApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	// Ray trace scene
	context.TransitionResource(m_colorTarget, ResourceState::UnorderedAccess);

	context.BindDescriptorHeap(GetDefaultDescriptorHeap());

	auto& computeContext = context.GetComputeContext();

	computeContext.SetRootSignature(m_globalRootSignature);
	computeContext.SetRayTracingPipeline(m_rayTracingPipeline);
	computeContext.SetRootSRV(0, m_rayTracingTLAS);
	//computeContext.SetDescriptors(0, m_testSet);
	computeContext.SetDescriptors(1, m_descriptorSet);

	computeContext.DispatchRays(m_shaderBindingTable, GetWindowWidth(), GetWindowHeight());

	// Copy ray tracing results to back buffer
	context.CopyColorBuffer(GetColorBuffer(), m_colorTarget);

	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
	
}


void RayTracingBasicApp::CreateDeviceDependentResources()
{
	// Create any resources that depend on the device, but not the window size

	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		256.0f);
	m_camera.SetPosition(Vector3(0.0f, 0.0f, -2.5f));

	m_rayGenConstantBuffer = CreateConstantBuffer("Ray Gen Constant Buffer", 1, sizeof(RayGenConstants));

	InitRayTracingBLAS();
	InitRayTracingTLAS();
	InitRootSignature();
	InitRayTracingPipeline();
	InitShaderBindingTable();
}


void RayTracingBasicApp::CreateWindowSizeDependentResources()
{
	// Create any resources that depend on window size.  May be called when the window size changes.
	InitColorTarget();
	InitDescriptorSet();
	m_camera.SetAspectRatio(GetWindowAspectRatio());
}


void RayTracingBasicApp::InitRayTracingBLAS()
{
	// Vertex buffer for a single triangle
	struct Vertex
	{
		float pos[3];
	};

	vector<Vertex> vertices = {
		{ {  1.0f,  1.0f, 0.0f } },
		{ { -1.0f,  1.0f, 0.0f } },
		{ {  0.0f, -1.0f, 0.0f } }
	};

	m_vertexBuffer = CreateRayTracingVertexBuffer<Vertex>("Vertex buffer", vertices);

	// Setup index buffer for a single triangle
	vector<uint32_t> indices = { 0, 1, 2 };
	
	m_indexBuffer = CreateRayTracingIndexBuffer("Index buffer", indices);

	// Bottom-level geometry description
	RayTracingGeometryDesc blasDesc{
		.name			= "Triangle BLAS",
		.geometryType	= RayTracingGeometryType::Triangles,
		.flags			= RayTracingGeometryFlags::Opaque
	};
	blasDesc.triangles.transform3x4 = 0;
	blasDesc.triangles.indexFormat = Format::R32_UInt;
	blasDesc.triangles.vertexFormat = Format::RGB32_Float;
	blasDesc.triangles.indexCount = indices.size();
	blasDesc.triangles.vertexCount = vertices.size();
	blasDesc.triangles.vertexStride = sizeof(Vertex);
	blasDesc.triangles.indexBuffer = m_indexBuffer->GetGpuAddress();
	blasDesc.triangles.vertexBuffer = m_vertexBuffer->GetGpuAddress();

	m_triangleBLAS = GetDevice()->CreateBLAS(blasDesc);
}


void RayTracingBasicApp::InitRayTracingTLAS()
{
	RayTracingInstanceDesc instanceDesc{
		.transform				= Matrix4{ kIdentity },
		.instanceID				= 1,
		.instanceMask			= 0xFF,
		.accelerationStructure	= m_triangleBLAS->GetGpuAddress()
	};

	vector<RayTracingInstanceDesc> instances = { instanceDesc };

	RayTracingTLASDesc tlasDesc{
		.instances = instances
	};

	m_rayTracingTLAS = GetDevice()->CreateTLAS(tlasDesc);
}


void RayTracingBasicApp::InitRootSignature()
{
	RootSignatureDesc desc{
		.name				= "Global Root Signature",
		.rootParameters		= {
			RootSRV(0, ShaderStage::RayGeneration, 0, true /* isAccelerationStructure */),
			//Table({ RayTracingAccelStruct(0, 1)}, ShaderStage::RayGeneration),
			Table({ TextureUAV(1, 1), ConstantBuffer(2, 1) }, ShaderStage::RayGeneration)
		}
	};

	m_globalRootSignature = CreateRootSignature(desc);
}


void RayTracingBasicApp::InitRayTracingPipeline()
{
	RayTracingPipelineDesc desc{
		.rootSignature = m_globalRootSignature,
		.shaderLibrary = {
			{ ShaderType::RayGeneration, { .shaderFile = "RayGen", .entry = "raygen_main" }},
			{ ShaderType::Miss, { .shaderFile = "Miss", .entry = "miss_main" }},
			{ ShaderType::ClosestHit, { .shaderFile = "ClosestHit", .entry = "closesthit_main" }}
		},
		.shaderGroups = { ShaderGroupDesc(2) },
		.maxPayloadSize = 3 * sizeof(float),
		.maxRayHitAttributeSize = 2 * sizeof(float)
	};

	m_rayTracingPipeline = GetDevice()->CreateRayTracingPipeline(desc);
}


void RayTracingBasicApp::InitShaderBindingTable()
{
	m_shaderBindingTable = GetDevice()->CreateShaderBindingTable(m_rayTracingPipeline);
}


void RayTracingBasicApp::InitColorTarget()
{
	ColorBufferDesc colorDesc{
		.name		= "Ray Tracing Target",
		.width		= GetWindowWidth(),
		.height		= GetWindowHeight(),
		.format		= GetColorFormat()
	};
	m_colorTarget = CreateColorBuffer(colorDesc);
}


void RayTracingBasicApp::InitDescriptorSet()
{
	//m_testSet = m_globalRootSignature->CreateDescriptorSet(0, { m_rayTracingTLAS });
	m_descriptorSet = m_globalRootSignature->CreateDescriptorSet(1, { m_colorTarget, m_rayGenConstantBuffer });
}


void RayTracingBasicApp::UpdateConstantBuffer()
{
	m_rayGenConstants.viewInverseMatrix = Invert(m_camera.GetViewMatrix());
	m_rayGenConstants.projectionInverseMatrix = Invert(m_camera.GetProjectionMatrix());

	m_rayGenConstantBuffer->Update(sizeof(m_rayGenConstants), &m_rayGenConstants);
}