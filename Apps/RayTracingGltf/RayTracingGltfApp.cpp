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

#include "RayTracingGltfApp.h"

#include "FileSystem.h"
#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"
#include "Graphics\Device.h"

using namespace Luna;
using namespace Math;
using namespace std;


RayTracingGltfApp::RayTracingGltfApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{}


int RayTracingGltfApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void RayTracingGltfApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
	GetFileSystem()->AddSearchPath("..\\Data\\Models\\FlightHelmet\\glTF");

}


void RayTracingGltfApp::Startup()
{
	// Application initialization, after device creation
}


void RayTracingGltfApp::Shutdown()
{
	// Application cleanup on shutdown
}


void RayTracingGltfApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffer();
}


void RayTracingGltfApp::Render()
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
	computeContext.SetDescriptors(2, m_bindlessVertexBufferSet);
	computeContext.SetDescriptors(3, m_bindlessIndexBufferSet);
	computeContext.SetDescriptors(4, m_bindlessDiffuseTextureSet);
	computeContext.SetDescriptors(5, m_bindlessOcclusionTextureSet);
	computeContext.SetDescriptors(6, m_samplerSet);

	computeContext.DispatchRays(m_shaderBindingTable, GetWindowWidth(), GetWindowHeight());

	// Copy ray tracing results to back buffer
	context.CopyColorBuffer(GetColorBuffer(), m_colorTarget);

	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void RayTracingGltfApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		XMConvertToRadians(45.0f),
		GetWindowAspectRatio(),
		0.1f,
		256.0f);
	m_camera.SetPosition(Vector3(0.0f, 1.0f, 10.0f));

	m_sceneConstantBuffer = CreateConstantBuffer("Scene Constant Buffer", 1, sizeof(SceneConstants));
	m_sampler = CreateSampler(CommonStates::SamplerLinearWrap());

	LoadAssets();
	InitRayTracingBLAS();
	InitRayTracingTLAS();
	InitRootSignature();
	InitRayTracingPipeline();
	InitShaderBindingTable();

	m_controller.SetSpeedScale(0.01f);
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.RefreshFromCamera();
	m_controller.SetOrbitTarget(m_model->boundingBox.GetCenter(), Length(m_camera.GetPosition()), 0.0001f);
}


void RayTracingGltfApp::CreateWindowSizeDependentResources()
{
	// Create any resources that depend on window size.  May be called when the window size changes.
	InitColorTarget();
	InitDescriptorSets();
	m_camera.SetAspectRatio(GetWindowAspectRatio());
}


void RayTracingGltfApp::LoadAssets()
{
	auto layout = VertexLayout<VertexComponent::PositionNormalTexcoord>();
	const ModelLoad flags = ModelLoad::StandardDefault | ModelLoad::RayTracingInput | ModelLoad::Force32BitIndices;
	const bool bLoadMaterials = true;
	m_model = LoadModel("FlightHelmet/glTF/FlightHelmet.gltf", layout, 10.0f, flags, bLoadMaterials);
	
	for (const auto& material : m_model->materials)
	{
		assert(material.diffuseTexture.IsValid());
		m_diffuseTextures.push_back(material.diffuseTexture);

		assert(material.metallicRoughnessTexture.IsValid());
		m_occlusionTextures.push_back(material.metallicRoughnessTexture);
	}

	assert(m_diffuseTextures.size() == m_occlusionTextures.size());
	assert(m_diffuseTextures.size() == m_model->meshes.size());
}


void RayTracingGltfApp::InitRayTracingBLAS()
{
	m_vertexBuffers.clear();
	m_indexBuffers.clear();

	std::vector<RayTracingGeometryDesc> geometries;

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

		geometries.push_back(blasDesc);

		m_vertexBuffers.push_back(mesh->vertexBuffer);
		m_indexBuffers.push_back(mesh->indexBuffer);
	}

	m_blasBuffer = m_device->CreateBLAS(geometries, RayTracingAccelerationStructureFlags::FastTrace);
}


void RayTracingGltfApp::InitRayTracingTLAS()
{
	Matrix4 transformMatrix{ kIdentity };;
	//transformMatrix.SetY(Vector4(0.0f, -1.0f, 0.0f, 0.0f));

	RayTracingInstanceDesc instanceDesc{
		.blas			= m_blasBuffer,
		.transform		= transformMatrix,
		.instanceID		= 0,
		.instanceMask	= 0xFF,
	};

	std::vector<RayTracingInstanceDesc> instances = { instanceDesc };
	RayTracingTLASDesc desc{ 
		.instances	= instances,
		.buildFlags = RayTracingAccelerationStructureFlags::FastTrace
	};

	m_tlasBuffer = m_device->CreateTLAS(desc);
}


void RayTracingGltfApp::InitRootSignature()
{
	const uint32_t numMeshes = (uint32_t)m_model->meshes.size();

	RootSignatureDesc desc{
		.name = "Global Root Signature",
		.rootParameters = {
			RootSRV(0, ShaderStage::AllRaytracing, 0, true /* isAccelerationStructure */),
			Table({ TextureUAV, ConstantBuffer }, ShaderStage::AllRaytracing),
			Table({ BindlessStructuredBufferSRV(0, numMeshes, 2) }, ShaderStage::ClosestHit | ShaderStage::AnyHit),
			Table({ BindlessStructuredBufferSRV(1, numMeshes, 3) }, ShaderStage::ClosestHit | ShaderStage::AnyHit),
			Table({ BindlessTextureSRV(2, numMeshes, 4) }, ShaderStage::ClosestHit | ShaderStage::AnyHit),
			Table({ BindlessTextureSRV(3, numMeshes, 5) }, ShaderStage::ClosestHit | ShaderStage::AnyHit),
			Table({ Sampler }, ShaderStage::ClosestHit | ShaderStage::AnyHit)
		}
	};

	m_rootSignature = CreateRootSignature(desc);
}


void RayTracingGltfApp::InitRayTracingPipeline()
{
	RayTracingPipelineDesc desc{
		.rootSignature = m_rootSignature,
		.rayGenShaders = {
			{ ShaderType::RayGeneration, { .shaderFile = "RayGen", .entry = "raygen_main" } }
		},
		.missShaders = {
			{ ShaderType::Miss, { .shaderFile = "Miss", .entry = "miss_main" } },
			{ ShaderType::Miss, { .shaderFile = "Shadow", .entry = "shadow_main" } }
		},
		.hitGroupShaders = {
			{ ShaderType::ClosestHit,	{ .shaderFile = "ClosestHit", .entry = "closesthit_main" } },
			{ ShaderType::AnyHit,		{ .shaderFile = "AnyHit", .entry = "anyhit_main" } }
		},
		.hitGroups = { ShaderGroupDesc(0, 1) },  // Indices 0 and 1 in the hitGroupShaders list
		.maxRecursionDepth = 4,
		.maxPayloadSize = 24,		// 24 bytes
		.maxRayHitAttributeSize = 2 * sizeof(float) // float2 barycentrics
	};

	m_rayTracingPipeline = m_device->CreateRayTracingPipeline(desc);
}


void RayTracingGltfApp::InitShaderBindingTable()
{
	ShaderBindingTableDesc desc{
		.rayGenShaders	= { ShaderRecord{.id = 0 } }, // refers to the RayGen shader at index 0 in the library
		.missShaders	= { ShaderRecord{.id = 0 }, ShaderRecord{.id = 1 } }, // refers to the Miss shaders at indices 0 and 1 in the library
		.hitGroups		= { ShaderRecord{.id = 0 } } // refers to the HitGroup at index 0 in the library
	};

	m_shaderBindingTable = m_device->CreateShaderBindingTable(desc, m_rayTracingPipeline);
}


void RayTracingGltfApp::InitColorTarget()
{
	ColorBufferDesc colorDesc{
		.name		= "Ray Tracing Target",
		.width		= GetWindowWidth(),
		.height		= GetWindowHeight(),
		.format		= GetColorFormat()
	};
	m_colorTarget = CreateColorBuffer(colorDesc);
}


void RayTracingGltfApp::InitDescriptorSets()
{
	m_descriptorSet = m_rootSignature->CreateDescriptorSet(1, { m_colorTarget, m_sceneConstantBuffer });

	m_bindlessVertexBufferSet = m_rootSignature->CreateDescriptorSet(2, m_vertexBuffers);
	m_bindlessIndexBufferSet = m_rootSignature->CreateDescriptorSet(3, m_indexBuffers);
	m_bindlessDiffuseTextureSet = m_rootSignature->CreateDescriptorSet(4, m_diffuseTextures);
	m_bindlessOcclusionTextureSet = m_rootSignature->CreateDescriptorSet(5, m_occlusionTextures);
	m_samplerSet = m_rootSignature->CreateDescriptorSet(6, m_sampler);
}


void RayTracingGltfApp::UpdateConstantBuffer()
{
	m_sceneConstants.viewInverse = Invert(m_camera.GetViewMatrix());
	m_sceneConstants.projInverse = Invert(m_camera.GetProjectionMatrix());
	m_sceneConstants.projectionToWorld = Invert(m_camera.GetViewProjectionMatrix());
	m_sceneConstants.cameraPosition = m_camera.GetPosition();

	if (!m_controller.DidUpdate())
	{
		m_sceneConstants.frame++;
	}
	else
	{
		m_sceneConstants.frame = 0;
	}

	m_sceneConstantBuffer->Update(sizeof(m_sceneConstants), &m_sceneConstants);
}