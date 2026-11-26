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

#include "RayQueryApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"
#include "Graphics\Device.h"


using namespace Luna;
using namespace Math;
using namespace std;


RayQueryApp::RayQueryApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{
}


int RayQueryApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void RayQueryApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void RayQueryApp::Startup()
{
	// Application initialization, after device creation
}


void RayQueryApp::Shutdown()
{
	// Application cleanup on shutdown
}


void RayQueryApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	UpdateConstantBuffer();
}


void RayQueryApp::Render()
{
	auto& context = GraphicsContext::Begin("Frame");

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(GetDepthBuffer(), ResourceState::DepthWrite);
	context.ClearColor(GetColorBuffer());
	context.ClearDepth(GetDepthBuffer());

	context.BeginRendering(GetColorBuffer(), GetDepthBuffer());

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());

	context.BindDescriptorHeap(GetDefaultDescriptorHeap());

	context.SetRootSignature(m_rootSignature);
	context.SetGraphicsPipeline(m_graphicsPipeline);

	context.SetDescriptors(0, m_descriptorSet);
	context.SetRootSRV(1, m_tlasBuffer);

	m_model->Render(context);

	context.EndRendering();

	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void RayQueryApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		XMConvertToRadians(45.0f),
		GetWindowAspectRatio(),
		0.1f,
		256.0f);
	m_camera.SetPosition(Vector3(3.0f, 7.0f, 16.0f));

	m_sceneConstantBuffer = CreateConstantBuffer("Scene Constant Buffer", 1, sizeof(SceneConstants));
	m_sceneConstants.lightPos = Vector3(40.0f, 50.0f, 25.0f);

	LoadAssets();
	InitRayTracingBLAS();
	InitRayTracingTLAS();
	InitRootSignature();

	m_controller.SetSpeedScale(0.01f);
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.RefreshFromCamera();
	m_controller.SetOrbitTarget(m_model->boundingBox.GetCenter(), Length(m_camera.GetPosition()), 4.0f);
}


void RayQueryApp::CreateWindowSizeDependentResources()
{
	if (!m_pipelineCreated)
	{
		InitGraphicsPipeline();
		m_pipelineCreated = true;
	}

	InitDescriptorSet();
	m_camera.SetAspectRatio(GetWindowAspectRatio());
}


void RayQueryApp::LoadAssets()
{
	auto layout = VertexLayout<VertexComponent::PositionNormalColorTexcoord>();
	const ModelLoad flags = ModelLoad::StandardDefault | ModelLoad::RayTracingInput | ModelLoad::Force32BitIndices;
	m_model = LoadModel("vulkanscene_shadow.gltf", layout, 1.0f, flags);
}


void RayQueryApp::InitRayTracingBLAS()
{
	m_vertexBuffers.clear();
	m_indexBuffers.clear();

	std::vector<RayTracingGeometryDesc> geometries;

	for (auto mesh : m_model->meshes)
	{
		// Bottom-level geometry description
		RayTracingGeometryDesc blasDesc{
			.name = std::format("{} BLAS", mesh->name),
			.geometryType = RayTracingGeometryType::Triangles,
			.flags = RayTracingGeometryFlags::Opaque,
			.buildFlags = RayTracingAccelerationStructureFlags::FastTrace
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


void RayQueryApp::InitRayTracingTLAS()
{
	Matrix4 transformMatrix{ kIdentity };;

	RayTracingInstanceDesc instanceDesc{
		.blas = m_blasBuffer,
		.transform = transformMatrix,
		.instanceID = 0,
		.instanceMask = 0xFF,
	};

	std::vector<RayTracingInstanceDesc> instances = { instanceDesc };
	RayTracingTLASDesc desc{
		.instances = instances,
		.buildFlags = RayTracingAccelerationStructureFlags::FastTrace
	};

	m_tlasBuffer = m_device->CreateTLAS(desc);
}


void RayQueryApp::InitRootSignature()
{
	RootSignatureDesc desc{
		.name				= "Root Signature",
		.rootParameters		= {
			Table({ ConstantBuffer }, ShaderStage::Vertex),
			RootSRV(0, ShaderStage::Pixel, 0, true /* isAccelerationStructure */)
		}
	};

	m_rootSignature = CreateRootSignature(desc);
}


void RayQueryApp::InitGraphicsPipeline()
{
	auto vertexLayout = VertexLayout<VertexComponent::PositionNormalColorTexcoord>();

	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= vertexLayout.GetSizeInBytes(),
		.inputClassification	= InputClassification::PerVertexData
	};

	GraphicsPipelineDesc desc{
		.name				= "Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState	= CommonStates::RasterizerDefault(),
		.rtvFormats			= { GetColorFormat() },
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "RayQueryVS" },
		.pixelShader		= { .shaderFile = "RayQueryPS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= vertexLayout.GetElements(),
		.rootSignature		= m_rootSignature
	};
	m_graphicsPipeline = CreateGraphicsPipeline(desc);
}


void RayQueryApp::InitDescriptorSet()
{
	m_descriptorSet = m_rootSignature->CreateDescriptorSet(0, m_sceneConstantBuffer);
}


void RayQueryApp::UpdateConstantBuffer()
{
	m_sceneConstants.projection = m_camera.GetProjectionMatrix();
	m_sceneConstants.view = m_camera.GetViewMatrix();
	m_sceneConstants.model = Matrix4{ kIdentity };

	// Rotate the light around Y axis.
	{
		float secondsToRotateAround = 16.0f;
		float elapsedTime = (float)m_timer.GetElapsedSeconds();
		float angleToRotateBy = 360.0f * (elapsedTime / secondsToRotateAround);
		Matrix4 rotate = Matrix3::MakeYRotation(XMConvertToRadians(angleToRotateBy));
		const Vector3& prevLightPosition = m_sceneConstants.lightPos;
		m_sceneConstants.lightPos = rotate * prevLightPosition;
	}

	m_sceneConstantBuffer->Update(sizeof(SceneConstants), &m_sceneConstants);
}
