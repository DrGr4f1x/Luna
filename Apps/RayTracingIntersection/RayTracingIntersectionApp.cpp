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

#include "RayTracingIntersectionApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"
#include "Graphics\Device.h"

using namespace Luna;
using namespace Math;
using namespace std;


RayTracingIntersectionApp::RayTracingIntersectionApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{}


int RayTracingIntersectionApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void RayTracingIntersectionApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void RayTracingIntersectionApp::Startup()
{
	// Application initialization, after device creation
}


void RayTracingIntersectionApp::Shutdown()
{
	// Application cleanup on shutdown
}


void RayTracingIntersectionApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffer();
}


void RayTracingIntersectionApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	// Ray trace scene
	context.TransitionResource(m_colorTarget, ResourceState::UnorderedAccess);

	context.BindDescriptorHeap(GetDefaultDescriptorHeap());

	auto& computeContext = context.GetComputeContext();

	computeContext.SetRootSignature(m_globalRootSignature);
	computeContext.SetRayTracingPipeline(m_rayTracingPipeline);
	computeContext.SetRootSRV(0, m_tlasBuffer);
	computeContext.SetDescriptors(1, m_descriptorSet);

	computeContext.DispatchRays(m_shaderBindingTable, GetWindowWidth(), GetWindowHeight());

	// Copy ray tracing results to back buffer
	context.CopyColorBuffer(GetColorBuffer(), m_colorTarget);

	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void RayTracingIntersectionApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		512.0f);
	m_camera.SetPosition(Vector3(0.0f, 0.0f, -60.0f));

	m_sceneConstantBuffer = CreateConstantBuffer("Scene Constant Buffer", 1, sizeof(SceneConstants));
	m_sceneConstants.lightPos = Vector4(60.0f, 0.0f, 60.0f, 0.0f);

	InitGeometry();
	InitRayTracingBLAS();
	InitRayTracingTLAS();
	InitRootSignature();
	InitRayTracingPipeline();
	InitShaderBindingTable();

	m_controller.RefreshFromCamera();
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.SetOrbitTarget(Vector3(0.0f, 0.0f, 0.0f), 60.0f, 0.25f);
	m_controller.SlowMovement(true);
	m_controller.SlowRotation(false);
	m_controller.SetSpeedScale(0.25f);
}


void RayTracingIntersectionApp::CreateWindowSizeDependentResources()
{
	// Create any resources that depend on window size.  May be called when the window size changes.
	InitColorTarget();
	InitDescriptorSet();
	m_camera.SetAspectRatio(GetWindowAspectRatio());
}


void RayTracingIntersectionApp::InitGeometry()
{
	// Generate spheres
	vector<Sphere> spheres;
	spheres.reserve(m_numSpheres);
	for (uint32_t i = 0; i < m_numSpheres; ++i)
	{
		Sphere sphere{};
		float radius = g_rng.NextFloat(1.0f, 2.0f);
		sphere.color = { g_rng.NextFloat(), g_rng.NextFloat(), g_rng.NextFloat(), 1.0f };

		// Get a random point in sphere
		float x, y, z, d = 0.0f;
		do 
		{
			x = 2.0f * g_rng.NextFloat() - 1.0f;
			y = 2.0f * g_rng.NextFloat() - 1.0f;
			z = 2.0f * g_rng.NextFloat() - 1.0f;
			d = x * x + y * y + z * z;
		} while (d > 1.0f);
		sphere.centerRadius = { x * 25.0f, y * 25.0f, z * 25.0f, radius };
		spheres.push_back(sphere);
	}

	GpuBufferDesc sphereBufferDesc{
		.name = "Sphere Buffer",
		.resourceType = ResourceType::StructuredBuffer,
		.memoryAccess = MemoryAccess::GpuRead,
		.elementCount = spheres.size(),
		.elementSize = sizeof(Sphere),
		.initialData = spheres.data()
	};

	m_sphereBuffer = CreateGpuBuffer(sphereBufferDesc);

	// Compute AABBs
	vector<AABB> aabbs;
	aabbs.reserve(m_numSpheres);
	for (uint32_t i = 0; i < m_numSpheres; ++i)
	{
		const Sphere& sphere = spheres[i];
		Vector3 center{ sphere.centerRadius.GetX(), sphere.centerRadius.GetY(), sphere.centerRadius.GetZ() };
		Vector3 radius{ sphere.centerRadius.GetW() };

		AABB aabb;
		Vector3 min = center - radius;
		Vector3 max = center + radius;
		XMStoreFloat3(&aabb.min, min);
		XMStoreFloat3(&aabb.max, max);
		aabbs.push_back(aabb);
	}

	GpuBufferDesc aabbBufferDesc{
		.name			= "AABB Buffer",
		.resourceType	= ResourceType::StructuredBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= aabbs.size(),
		.elementSize	= sizeof(AABB),
		.initialData	= aabbs.data(),
		.bRayTracingBuildInput = true
	};

	assert(spheres.size() == aabbs.size());

	m_aabbBuffer = CreateGpuBuffer(aabbBufferDesc);
}


void RayTracingIntersectionApp::InitRayTracingBLAS()
{
	// Bottom-level geometry description
	RayTracingGeometryDesc blasDesc{
		.name			= "Cube BLAS",
		.geometryType	= RayTracingGeometryType::AABBs,
		.flags			= RayTracingGeometryFlags::Opaque,
		.buildFlags		= RayTracingAccelerationStructureFlags::FastTrace
	};

	blasDesc.aabbs.aabbBuffer = m_aabbBuffer->GetGpuAddress();
	blasDesc.aabbs.aabbCount = m_numSpheres;
	blasDesc.aabbs.aabbStride = sizeof(AABB);

	m_blasBuffer = GetDevice()->CreateBLAS(blasDesc);
}


void RayTracingIntersectionApp::InitRayTracingTLAS()
{
	RayTracingInstanceDesc instanceDesc{
		.blas			= m_blasBuffer,
		.transform		= Matrix4{ kIdentity },
		.instanceID		= 0,
		.instanceMask	= 0xFF,
	};

	vector<RayTracingInstanceDesc> instances = { instanceDesc };

	RayTracingTLASDesc tlasDesc{
		.instances = instances
	};

	m_tlasBuffer = GetDevice()->CreateTLAS(tlasDesc);
}


void RayTracingIntersectionApp::InitRootSignature()
{
	RootSignatureDesc desc{
		.name = "Global Root Signature",
		.rootParameters = {
			RootSRV(0, ShaderStage::RayGeneration, 0, true /* isAccelerationStructure */),
			Table({ TextureUAV(1, 1), ConstantBuffer(2, 1), StructuredBufferSRV(1, 1) }, ShaderStage::AllRaytracing)
		}
	};

	m_globalRootSignature = CreateRootSignature(desc);
}


void RayTracingIntersectionApp::InitRayTracingPipeline()
{
	RayTracingPipelineDesc desc{
		.rootSignature = m_globalRootSignature,
		.rayGenShaders = {
			{ ShaderType::RayGeneration, {.shaderFile = "RayGen", .entry = "raygen_main" }}
		},
		.missShaders = {
			{ ShaderType::Miss, {.shaderFile = "Miss", .entry = "miss_main" }},
		},
		.hitGroupShaders = {
			{ ShaderType::ClosestHit, {.shaderFile = "ClosestHit", .entry = "closesthit_main" } },
			{ ShaderType::Intersection, {.shaderFile = "Intersection", .entry = "intersection_main"} }
		},
		.hitGroups = { ShaderGroupDesc(0, 1) },
		.maxPayloadSize = 3 * sizeof(float),
		.maxRayHitAttributeSize = 1 * sizeof(float)
	};

	m_rayTracingPipeline = GetDevice()->CreateRayTracingPipeline(desc);
}


void RayTracingIntersectionApp::InitShaderBindingTable()
{
	ShaderBindingTableDesc desc{
		.rayGenShaders	= { ShaderRecord{.id = 0 } }, // refers to the RayGen shader at index 0 in the library
		.missShaders	= { ShaderRecord{.id = 0 } }, // refers to the Miss shader at index 0 in the library
		.hitGroups		= { ShaderRecord{.id = 0 } }  // refers to the HitGroup at index 0 in the library
	};

	m_shaderBindingTable = GetDevice()->CreateShaderBindingTable(desc, m_rayTracingPipeline);
}


void RayTracingIntersectionApp::InitColorTarget()
{
	ColorBufferDesc colorDesc{
		.name = "Ray Tracing Target",
		.width = GetWindowWidth(),
		.height = GetWindowHeight(),
		.format = GetColorFormat()
	};
	m_colorTarget = CreateColorBuffer(colorDesc);
}


void RayTracingIntersectionApp::InitDescriptorSet()
{
	m_descriptorSet = m_globalRootSignature->CreateDescriptorSet(1, { m_colorTarget, m_sceneConstantBuffer, m_sphereBuffer });
}


void RayTracingIntersectionApp::UpdateConstantBuffer()
{
	m_sceneConstants.viewInverse = Invert(m_camera.GetViewMatrix());
	m_sceneConstants.projInverse = Invert(m_camera.GetProjectionMatrix());

	// Rotate the light around Y axis.
	{
		float secondsToRotateAround = 16.0f;
		float elapsedTime = (float)m_timer.GetElapsedSeconds();
		float angleToRotateBy = 360.0f * (elapsedTime / secondsToRotateAround);
		Matrix4 rotate = Matrix3::MakeYRotation(XMConvertToRadians(angleToRotateBy));
		const Vector4& prevLightPosition = m_sceneConstants.lightPos;
		m_sceneConstants.lightPos = rotate * prevLightPosition;
	}

	m_sceneConstantBuffer->Update(sizeof(m_sceneConstants), &m_sceneConstants);
}