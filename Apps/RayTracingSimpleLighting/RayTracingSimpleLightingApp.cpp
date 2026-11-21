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
	float elapsedTime = (float)m_timer.GetElapsedSeconds();

	// Rotate the camera around Y axis.
	{
		float secondsToRotateAround = 24.0f;
		float angleToRotateBy = -360.0f * (elapsedTime / secondsToRotateAround);
		Matrix4 rotate = Matrix3::MakeYRotation(XMConvertToRadians(angleToRotateBy));
		m_eye = rotate * m_eye;
		m_up = rotate * m_up;
		m_at = rotate * m_at;
		UpdateCameraMatrices();
	}

	// Rotate the second light around Y axis.
	{
		float secondsToRotateAround = 8.0f;
		float angleToRotateBy = 360.0f * (elapsedTime / secondsToRotateAround);
		Matrix4 rotate = Matrix3::MakeYRotation(XMConvertToRadians(angleToRotateBy));
		const Vector4& prevLightPosition = m_sceneConstants.lightPosition;
		m_sceneConstants.lightPosition = rotate * prevLightPosition;
	}

	UpdateConstantBuffer();
}


void RayTracingSimpleLightingApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	// Ray trace scene
	context.TransitionResource(m_colorTarget, ResourceState::UnorderedAccess);

	context.BindDescriptorHeap(GetDefaultDescriptorHeap());

	auto& computeContext = context.GetComputeContext();

	computeContext.SetRootSignature(m_globalRootSignature);
	computeContext.SetRayTracingPipeline(m_rayTracingPipeline);
	computeContext.SetDescriptors(0, m_colorTargetDescriptorSet);
	computeContext.SetRootSRV(1, m_rayTracingTLAS);
	computeContext.SetRootCBV(2, m_sceneConstantBuffer);
	computeContext.SetDescriptors(3, m_geometryBufferDescriptorSet);

	computeContext.DispatchRays(m_shaderBindingTable, GetWindowWidth(), GetWindowHeight());

	// Copy ray tracing results to back buffer
	context.CopyColorBuffer(GetColorBuffer(), m_colorTarget);

	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void RayTracingSimpleLightingApp::CreateDeviceDependentResources()
{
	// Setup camera.
	{
		// Initialize the view and projection inverse matrices.
		m_eye = Vector4{ 0.0f, 2.0f, -5.0f, 1.0f };
		m_at = Vector4{ 0.0f, 0.0f, 0.0f, 1.0f };
		Vector4 right{ 1.0f, 0.0f, 0.0f, 0.0f };

		Vector4 direction = Normalize(m_at - m_eye);
		m_up = Vector4(Normalize(Cross(Vector3(direction), Vector3(right))), 1.0f);

		// Rotate camera around Y axis.
		Matrix4 rotate = Matrix3::MakeYRotation(XMConvertToRadians(45.0f));
		m_eye = rotate * m_eye;
		m_up = rotate * m_up;

		UpdateCameraMatrices();
	}

	// Setup lights.
	{
		// Initialize the lighting parameters.
		m_sceneConstants.lightPosition = Vector4{ 0.0f, 1.8f, -3.0f, 0.0f };
		m_sceneConstants.lightAmbientColor = Vector4{ 0.5f, 0.5f, 0.5f, 1.0f };
		m_sceneConstants.lightDiffuseColor = Vector4{ 0.5f, 0.0f, 0.0f, 1.0f };
	}

	// Setup material
	m_cubeConstants.albedo = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

	// Create scene constant buffer
	m_sceneConstantBuffer = CreateConstantBuffer("Scene Constant Buffer", 1, sizeof(SceneConstants));

	InitGeometry();
	InitRayTracingBLAS();
	InitRayTracingTLAS();
	InitRootSignatures();
	InitRayTracingPipeline();
	InitShaderBindingTable();
}


void RayTracingSimpleLightingApp::CreateWindowSizeDependentResources()
{
	InitColorTarget();
	InitDescriptorSets();
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

	GpuBufferDesc indexBufferDesc{
		.name			= "Index Buffer",
		.resourceType	= ResourceType::IndexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= _countof(indices),
		.elementSize	= sizeof(uint16_t),
		.initialData	= indices,
		.bAllowShaderResource	= true,
		.bRayTracingBuildInput	= true
	};

	m_indexBuffer = GetDevice()->CreateGpuBuffer(indexBufferDesc);

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

	GpuBufferDesc vertexBufferDesc{
		.name			= "Vertex Buffer",
		.resourceType	= ResourceType::VertexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= _countof(vertices),
		.elementSize	= sizeof(Vertex),
		.initialData	= vertices,
		.bAllowShaderResource	= true,
		.bRayTracingBuildInput	= true
	};

	m_vertexBuffer = GetDevice()->CreateGpuBuffer(vertexBufferDesc);
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
			RootCBV(0, ShaderStage::AllRaytracing),
			Table( { RawBufferSRV(1), StructuredBufferSRV(2) }, ShaderStage::AllRaytracing)
		}
	};

	m_globalRootSignature = CreateRootSignature(globalDesc);

	RootSignatureDesc localDesc{
		.name				= "Local Root Signature",
		.rootParameters		= {
			RootConstants(1, (uint32_t)SizeOfInUint32<CubeConstants>(), ShaderStage::AllRaytracing)
		},
		.localRootSignature = true
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
		.maxPayloadSize	= 4 * sizeof(float),		// float4 pixelColor
		.maxRayHitAttributeSize = 2 * sizeof(float) // float2 barycentrics
	};

	m_rayTracingPipeline = GetDevice()->CreateRayTracingPipeline(desc);
}


void RayTracingSimpleLightingApp::InitShaderBindingTable()
{
	ShaderBindingTableDesc desc{
		.rayGenShaders	= { ShaderRecord{ .id = 0 } }, // refers to the RayGen shader at index 0 in the library
		.missShaders	= { ShaderRecord{ .id = 0 } }, // refers to the Miss shader at index 0 in the library
		.hitGroups		= { 
			ShaderRecord{ .id = 0, .data = &m_cubeConstants, .size = sizeof(CubeConstants) } // refers to the HitGroup at index 0 in the library
		},
		.hitGroupDataSize	= sizeof(CubeConstants)
	};

	m_shaderBindingTable = GetDevice()->CreateShaderBindingTable(desc, m_rayTracingPipeline);
}


void RayTracingSimpleLightingApp::InitColorTarget()
{
	ColorBufferDesc colorDesc{
		.name	= "Ray Tracing Target",
		.width	= GetWindowWidth(),
		.height = GetWindowHeight(),
		.format = GetColorFormat()
	};
	m_colorTarget = CreateColorBuffer(colorDesc);
}


void RayTracingSimpleLightingApp::InitDescriptorSets()
{
	m_colorTargetDescriptorSet = m_globalRootSignature->CreateDescriptorSet(0, { m_colorTarget});
	m_geometryBufferDescriptorSet = m_globalRootSignature->CreateDescriptorSet(3, { m_indexBuffer, m_vertexBuffer });
}


void RayTracingSimpleLightingApp::UpdateCameraMatrices()
{
	m_sceneConstants.cameraPosition = m_eye;
	Matrix4 view = Matrix4::MakeLookAt(m_eye, m_at, m_up);
	const float fovY = 45.0f;
	Matrix4 proj = Matrix4::MakePerspective(XMConvertToRadians(fovY), 1.0f / GetWindowAspectRatio(), 1.0f, 125.0f);
	Matrix4 viewProj = proj * view;

	m_sceneConstants.projectionToWorld = Invert(viewProj);
}


void RayTracingSimpleLightingApp::UpdateConstantBuffer()
{
	m_sceneConstantBuffer->Update(sizeof(SceneConstants), &m_sceneConstants);
}