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

#include "RayTracingSimpleLightingApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"
#include "Graphics\Device.h"


using namespace Luna;
using namespace Math;
using namespace std;


RayTracingSimpleLightingApp::RayTracingSimpleLightingApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{
}


int RayTracingSimpleLightingApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void RayTracingSimpleLightingApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void RayTracingSimpleLightingApp::Startup()
{
	// Application initialization, after device creation
}


void RayTracingSimpleLightingApp::Shutdown()
{
	// Application cleanup on shutdown
}


void RayTracingSimpleLightingApp::Update()
{
	// Application update tick
	// Set m_bIsRunning to false if your application wants to exit
}


void RayTracingSimpleLightingApp::Render()
{
	// Application main render loop
	Application::Render();
}


void RayTracingSimpleLightingApp::CreateDeviceDependentResources()
{
	// Create any resources that depend on the device, but not the window size

	InitGeometry();
	InitRayTracingBLAS();
	InitRayTracingTLAS();
	InitRootSignatures();
	InitRayTracingPipeline();
}


void RayTracingSimpleLightingApp::CreateWindowSizeDependentResources()
{
	// Create any resources that depend on window size.  May be called when the window size changes.
}


void RayTracingSimpleLightingApp::InitGeometry()
{
	// Cube indices.
	uint16_t indices[] =
	{
		3,1,0,
		2,1,3,

		6,4,5,
		7,4,6,

		11,9,8,
		10,9,11,

		14,12,13,
		15,12,14,

		19,17,16,
		18,17,19,

		22,20,21,
		23,20,22
	};

	// Cube vertices positions and corresponding triangle normals.
	Vertex vertices[] =
	{
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f) },

		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f) },

		{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
	};

	m_indexBuffer = CreateRayTracingIndexBuffer("Index buffer", indices);
	m_vertexBuffer = CreateRayTracingVertexBuffer<Vertex>("Vertex buffer", vertices);
}


void RayTracingSimpleLightingApp::InitRayTracingBLAS()
{
	// Bottom-level geometry description
	RayTracingGeometryDesc blasDesc{
		.name			= "Cube BLAS",
		.geometryType	= RayTracingGeometryType::Triangles,
		.flags			= RayTracingGeometryFlags::Opaque,
		.buildFlags		= RayTracingAccelerationStructureFlags::FastTrace
	};
	blasDesc.triangles.transform3x4 = 0;
	blasDesc.triangles.indexFormat = Format::R16_UInt;
	blasDesc.triangles.vertexFormat = Format::RGB32_Float;
	blasDesc.triangles.indexCount = m_indexBuffer->GetElementCount();
	blasDesc.triangles.vertexCount = m_vertexBuffer->GetElementCount();
	blasDesc.triangles.vertexStride = sizeof(Vertex);
	blasDesc.triangles.indexBuffer = m_indexBuffer->GetGpuAddress();
	blasDesc.triangles.vertexBuffer = m_vertexBuffer->GetGpuAddress();

	m_cubeBLAS = GetDevice()->CreateBLAS(blasDesc);
}


void RayTracingSimpleLightingApp::InitRayTracingTLAS()
{
	RayTracingInstanceDesc instanceDesc{
		.blas			= m_cubeBLAS,
		.transform		= Matrix4{ kIdentity },
		.instanceID		= 1,
		.instanceMask	= 0xFF,
	};

	vector<RayTracingInstanceDesc> instances = { instanceDesc };

	RayTracingTLASDesc tlasDesc{
		.instances = instances
	};

	m_rayTracingTLAS = GetDevice()->CreateTLAS(tlasDesc);
}


void RayTracingSimpleLightingApp::InitRootSignatures()
{
	RootSignatureDesc globalDesc{
		.name				= "Global Root Signature",
		.rootParameters		= {
			Table({ TextureUAV }, ShaderStage::AllRaytracing),
			RootSRV(0, ShaderStage::AllRaytracing, 0, true /* isAccelerationStructure */),
			Table( { RawBufferSRV(1), StructuredBufferSRV(2) }, ShaderStage::AllRaytracing),
			RootCBV(0, ShaderStage::AllRaytracing)
		}
	};

	m_globalRootSignature = CreateRootSignature(globalDesc);

	RootSignatureDesc localDesc{
		.name				= "Local Root Signature",
		.rootParameters		= {
			RootConstants(1, SizeOfInUint32<CubeConstants>())
		}
	};

	m_localRootSignature = CreateRootSignature(localDesc);
}


void RayTracingSimpleLightingApp::InitRayTracingPipeline()
{
	RayTracingPipelineDesc desc{
		.rootSignature = m_globalRootSignature,
		.rayGenShaders = {
			{ ShaderType::RayGeneration, { .shaderFile = "RayGen", .entry = "raygen_main" } }
		},
		.missShaders = {
			{ ShaderType::Miss, { .shaderFile = "Miss", .entry = "miss_main" } },
		},
		.hitGroupShaders = {
			{ ShaderType::ClosestHit, { .shaderFile = "ClosestHit", .entry = "closesthit_main" } }
		},
		.hitGroups = { ShaderGroupDesc(0, m_localRootSignature) },  // Index 0 in the hitGroupShaders list, i.e. the 'ClosestHit' shader
		.maxPayloadSize	= 3 * sizeof(float),
		.maxRayHitAttributeSize = 2 * sizeof(float)
	};

	m_rayTracingPipeline = GetDevice()->CreateRayTracingPipeline(desc);
}