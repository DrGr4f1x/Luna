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

#include "RayTracingShadowsApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"
#include "Graphics\Device.h"


using namespace Luna;
using namespace Math;
using namespace std;


RayTracingShadowsApp::RayTracingShadowsApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{
}


int RayTracingShadowsApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void RayTracingShadowsApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void RayTracingShadowsApp::Startup()
{
	// Application initialization, after device creation
}


void RayTracingShadowsApp::Shutdown()
{
	// Application cleanup on shutdown
}


void RayTracingShadowsApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffer();
}


void RayTracingShadowsApp::Render()
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
	computeContext.SetDescriptors(2, m_bindlessVertexBufferSet);
	computeContext.SetDescriptors(3, m_bindlessIndexBufferSet);

	computeContext.DispatchRays(m_shaderBindingTable, GetWindowWidth(), GetWindowHeight());

	// Copy ray tracing results to back buffer
	context.CopyColorBuffer(GetColorBuffer(), m_colorTarget);

	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void RayTracingShadowsApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		XMConvertToRadians(45.0f),
		GetWindowAspectRatio(),
		0.1f,
		256.0f);
	m_camera.SetPosition(Vector3(3.0f, 7.0f, 16.0f));

	m_sceneConstantBuffer = CreateConstantBuffer("Scene Constant Buffer", 1, sizeof(SceneConstants));
	m_sceneConstants.lightPos = Vector4(40.0f, 50.0f, 25.0f, 1.0f);

	LoadAssets();
	InitRayTracingBLAS();
	InitRayTracingTLAS();
	InitRootSignature();
	InitRayTracingPipeline();
	InitShaderBindingTable();

	m_controller.SetSpeedScale(0.01f);
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.RefreshFromCamera();
	m_controller.SetOrbitTarget(m_model->boundingBox.GetCenter(), Length(m_camera.GetPosition()), 4.0f);
}


void RayTracingShadowsApp::CreateWindowSizeDependentResources()
{
	// Create any resources that depend on window size.  May be called when the window size changes.
	InitColorTarget();
	InitDescriptorSets();
	m_camera.SetAspectRatio(GetWindowAspectRatio());
}


void RayTracingShadowsApp::LoadAssets()
{
	auto layout = VertexLayout<VertexComponent::PositionNormalColorTexcoord>();
	const ModelLoad flags = ModelLoad::StandardDefault | ModelLoad::RayTracingInput | ModelLoad::Force32BitIndices;
	m_model = LoadModel("vulkanscene_shadow.gltf", layout, 1.0f, flags);

	m_sceneConstants.vertexStride = layout.GetSizeInBytes();
}


void RayTracingShadowsApp::InitRayTracingBLAS()
{
	m_blasBuffers.clear();
	m_vertexBuffers.clear();
	m_indexBuffers.clear();

	for (auto mesh : m_model->meshes)
	{
		// Bottom-level geometry description
		RayTracingGeometryDesc blasDesc{
			.name			= std::format("{} BLAS", mesh->name),
			.geometryType	= RayTracingGeometryType::Triangles,
			.flags			= RayTracingGeometryFlags::Opaque,
			.buildFlags		= RayTracingAccelerationStructureFlags::FastTrace
		};
		blasDesc.triangles.transform3x4 = 0;
		blasDesc.triangles.indexFormat = Format::R32_UInt;
		blasDesc.triangles.vertexFormat = Format::RGB32_Float;
		blasDesc.triangles.indexCount = mesh->indexBuffer->GetElementCount();
		blasDesc.triangles.vertexCount = mesh->vertexBuffer->GetElementCount();
		blasDesc.triangles.vertexStride = mesh->vertexBuffer->GetElementSize();
		blasDesc.triangles.indexBuffer = mesh->indexBuffer->GetGpuAddress();
		blasDesc.triangles.vertexBuffer = mesh->vertexBuffer->GetGpuAddress();

		auto blasBuffer = m_device->CreateBLAS(blasDesc);
		m_blasBuffers.push_back(blasBuffer);

		m_vertexBuffers.push_back(mesh->vertexBuffer);
		m_indexBuffers.push_back(mesh->indexBuffer);
	}
}


void RayTracingShadowsApp::InitRayTracingTLAS()
{
	vector<RayTracingInstanceDesc> instances;
	instances.reserve(m_blasBuffers.size());

	uint32_t instanceID = 0;
	for (auto blasBuffer : m_blasBuffers)
	{
		RayTracingInstanceDesc instanceDesc{
			.blas			= blasBuffer,
			.transform		= Matrix4{ kIdentity },
			.instanceID		= instanceID,
			.instanceMask	= 0xFF,
		};
		instances.push_back(instanceDesc);
		++instanceID;
	}

	RayTracingTLASDesc tlasDesc{
		.instances = instances,
		.buildFlags = RayTracingAccelerationStructureFlags::FastTrace
	};

	m_tlasBuffer = m_device->CreateTLAS(tlasDesc);
}


void RayTracingShadowsApp::InitRootSignature()
{
	const uint32_t numMeshes = (uint32_t)m_model->meshes.size();

	RootSignatureDesc desc{
		.name = "Global Root Signature",
		.rootParameters = {
			RootSRV(0, ShaderStage::AllRaytracing, 0, true /* isAccelerationStructure */),
			Table({ TextureUAV(1, 1), ConstantBuffer(2, 1) }, ShaderStage::AllRaytracing),
			Table({ BindlessStructuredBufferSRV(1, numMeshes, 2) }, ShaderStage::ClosestHit),
			Table({ BindlessStructuredBufferSRV(2, numMeshes, 3) }, ShaderStage::ClosestHit)
		}
	};

	m_globalRootSignature = CreateRootSignature(desc);
}


void RayTracingShadowsApp::InitRayTracingPipeline()
{
	RayTracingPipelineDesc desc{
		.rootSignature = m_globalRootSignature,
		.rayGenShaders = {
			{ ShaderType::RayGeneration, { .shaderFile = "RayGen", .entry = "raygen_main" } }
		},
		.missShaders = {
			{ ShaderType::Miss, { .shaderFile = "Miss", .entry = "miss_main" } },
			{ ShaderType::Miss, { .shaderFile = "Shadow", .entry = "shadow_main"} }
		},
		.hitGroupShaders = {
			{ ShaderType::ClosestHit, {.shaderFile = "ClosestHit", .entry = "closesthit_main" } }
		},
		.hitGroups = { ShaderGroupDesc(0) },  // Index 0 in the hitGroupShaders list, i.e. the 'ClosestHit' shader
		.maxRecursionDepth = 2,
		.maxPayloadSize = 4 * sizeof(float),		// float4 pixelColor
		.maxRayHitAttributeSize = 2 * sizeof(float) // float2 barycentrics
	};

	m_rayTracingPipeline = m_device->CreateRayTracingPipeline(desc);
}


void RayTracingShadowsApp::InitShaderBindingTable()
{
	ShaderBindingTableDesc desc{
		.rayGenShaders	= { ShaderRecord{.id = 0 } }, // refers to the RayGen shader at index 0 in the library
		.missShaders	= { ShaderRecord{.id = 0 }, ShaderRecord{.id = 1 } }, // refers to the Miss shaders at indices 0 and 1 in the library
		.hitGroups		= { ShaderRecord{.id = 0  } } // refers to the HitGroup at index 0 in the library
	};

	m_shaderBindingTable = m_device->CreateShaderBindingTable(desc, m_rayTracingPipeline);
}


void RayTracingShadowsApp::InitColorTarget()
{
	ColorBufferDesc colorDesc{
		.name		= "Ray Tracing Target",
		.width		= GetWindowWidth(),
		.height		= GetWindowHeight(),
		.format		= GetColorFormat()
	};
	m_colorTarget = CreateColorBuffer(colorDesc);
}


void RayTracingShadowsApp::InitDescriptorSets()
{
	m_descriptorSet = m_globalRootSignature->CreateDescriptorSet(1, { m_colorTarget, m_sceneConstantBuffer });

	m_bindlessVertexBufferSet = m_globalRootSignature->CreateDescriptorSet(2, m_vertexBuffers);
	//m_bindlessVertexBufferSet->SetSRVs(1, m_vertexBuffers);

	m_bindlessIndexBufferSet = m_globalRootSignature->CreateDescriptorSet(3, m_indexBuffers);
	//m_bindlessIndexBufferSet->SetSRVs(2, m_indexBuffers);
}


void RayTracingShadowsApp::UpdateConstantBuffer()
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