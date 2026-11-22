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
	// Application update tick
	// Set m_bIsRunning to false if your application wants to exit
}


void RayTracingShadowsApp::Render()
{
	// Application main render loop
	Application::Render();
}


void RayTracingShadowsApp::CreateDeviceDependentResources()
{
	// Create any resources that depend on the device, but not the window size

	LoadAssets();
	InitRayTracingBLAS();
}


void RayTracingShadowsApp::CreateWindowSizeDependentResources()
{
	// Create any resources that depend on window size.  May be called when the window size changes.
}


void RayTracingShadowsApp::LoadAssets()
{
	auto layout = VertexLayout<VertexComponent::PositionNormalColorTexcoord>();
	const ModelLoad flags = ModelLoad::StandardDefault | ModelLoad::RayTracingInput | ModelLoad::Force32BitIndices;
	m_model = LoadModel("vulkanscene_shadow.gltf", layout, 1.0f, flags);
}


void RayTracingShadowsApp::InitRayTracingBLAS()
{
	m_blasBuffers.clear();
	m_blasBuffers.reserve(m_model->meshes.size());

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
		blasDesc.triangles.indexFormat = mesh->indexBuffer->GetFormat();
		blasDesc.triangles.vertexFormat = Format::RGB32_Float;
		blasDesc.triangles.indexCount = mesh->indexBuffer->GetElementCount();
		blasDesc.triangles.vertexCount = mesh->vertexBuffer->GetElementCount();
		blasDesc.triangles.vertexStride = mesh->vertexBuffer->GetElementSize();
		blasDesc.triangles.indexBuffer = mesh->indexBuffer->GetGpuAddress();
		blasDesc.triangles.vertexBuffer = mesh->vertexBuffer->GetGpuAddress();

		auto blasBuffer = GetDevice()->CreateBLAS(blasDesc);
		m_blasBuffers.push_back(blasBuffer);
	}
}


void RayTracingShadowsApp::InitRayTracingTLAS()
{
	vector<RayTracingInstanceDesc> instances;
	instances.reserve(m_blasBuffers.size());

	for (auto blasBuffer : m_blasBuffers)
	{
		RayTracingInstanceDesc instanceDesc{
			.blas			= blasBuffer,
			.transform		= Matrix4{ kIdentity },
			.instanceID		= 0,
			.instanceMask	= 0xFF,
		};
	}

	RayTracingTLASDesc tlasDesc{
		.instances = instances
	};

	m_rayTracingTLAS = GetDevice()->CreateTLAS(tlasDesc);
}


void RayTracingShadowsApp::InitRootSignature()
{
	RootSignatureDesc desc{
		.name = "Global Root Signature",
		.rootParameters = {
			RootSRV(0, ShaderStage::AllRaytracing, 0, true /* isAccelerationStructure */),
			Table({ TextureUAV(1, 1), ConstantBuffer(2, 1), StructuredBufferSRV(1, 2) }, ShaderStage::AllRaytracing)
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
		.maxPayloadSize = 4 * sizeof(float),		// float4 pixelColor
		.maxRayHitAttributeSize = 2 * sizeof(float) // float2 barycentrics
	};

	m_rayTracingPipeline = GetDevice()->CreateRayTracingPipeline(desc);
}


void RayTracingShadowsApp::InitShaderBindingTable()
{
	ShaderBindingTableDesc desc{
		.rayGenShaders	= { ShaderRecord{.id = 0 } }, // refers to the RayGen shader at index 0 in the library
		.missShaders	= { ShaderRecord{.id = 0 } }, // refers to the Miss shader at index 0 in the library
		.hitGroups		= { ShaderRecord{.id = 0  } } // refers to the HitGroup at index 0 in the library
	};

	m_shaderBindingTable = GetDevice()->CreateShaderBindingTable(desc, m_rayTracingPipeline);
}


void RayTracingShadowsApp::InitColorTarget()
{
	ColorBufferDesc colorDesc{
		.name = "Ray Tracing Target",
		.width = GetWindowWidth(),
		.height = GetWindowHeight(),
		.format = GetColorFormat()
	};
	m_colorTarget = CreateColorBuffer(colorDesc);
}


void RayTracingShadowsApp::InitDescriptorSet()
{
	m_descriptorSet = m_globalRootSignature->CreateDescriptorSet(1, { m_colorTarget, m_sceneConstantBuffer, m_sphereBuffer });
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