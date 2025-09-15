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

#include "MeshletInstancingApp.h"

#include "InputSystem.h"
#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"
#include "Graphics\Device.h"
#include "Graphics\DeviceCaps.h"
#include "Graphics\DeviceManager.h"

#include "Shaders\Shared.h"

using namespace Luna;
using namespace Math;
using namespace std;


// Limit our dispatch threadgroup count to 65536 for indexing simplicity.
const uint32_t c_maxGroupDispatchCount = 65536u;


MeshletInstancingApp::MeshletInstancingApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{}


int MeshletInstancingApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void MeshletInstancingApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void MeshletInstancingApp::Startup()
{
	if (!GetDevice()->GetDeviceCaps().features.meshShader)
	{
		Utility::ExitFatal("Mesh shader support is required for this sample.", "Missing feature");
	}
}


void MeshletInstancingApp::Shutdown()
{
	// Application cleanup on shutdown
}


void MeshletInstancingApp::Update()
{
	m_controller.Update(m_inputSystem.get(), (float)m_timer.GetElapsedSeconds(), m_mouseMoveHandled);

	int32_t newLevel = m_instanceLevel;

	if (m_inputSystem->IsFirstPressed(DigitalInput::kKey_add))
	{
		++newLevel;
		newLevel = min(newLevel, m_maxInstanceLevel);
	}
	else if (m_inputSystem->IsFirstPressed(DigitalInput::kKey_subtract))
	{
		--newLevel;
		newLevel = max(newLevel, 0);
	}

	if (newLevel != m_instanceLevel)
	{
		m_instanceLevel = newLevel;
		RegenerateInstances();
	}

	UpdateConstantBuffer();
}


void MeshletInstancingApp::Render()
{
	auto& context = GraphicsContext::Begin("Scene");

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.TransitionResource(GetDepthBuffer(), ResourceState::DepthWrite);
	context.ClearColor(GetColorBuffer());
	context.ClearDepth(GetDepthBuffer());

	context.BeginRendering(GetColorBuffer(), GetDepthBuffer());

	context.SetViewportAndScissor(0u, 0u, GetWindowWidth(), GetWindowHeight());

	context.SetRootSignature(m_rootSignature);
	context.SetMeshletPipeline(m_meshletPipeline);

	context.SetDescriptors(0, m_cbvDescriptorSet);
	context.SetDescriptors(3, m_instanceDescriptorSet);

	uint32_t meshIndex = 0;
	for (const auto& mesh : m_model)
	{
		//context.SetConstant(1, 2, mesh.indexSize);
		context.SetDescriptors(2, m_srvDescriptorSets[meshIndex]);

		for (uint32_t i = 0; i < (uint32_t)mesh.meshletSubsets.size(); ++i)
		{
			const auto& subset = mesh.meshletSubsets[i];

			const uint32_t packCount = mesh.GetLastMeshletPackCount(i, MAX_VERTS, MAX_PRIMS);
			const float groupsPerInstance = float(subset.count - 1) + 1.0f / packCount;

			const uint32_t maxInstancePerBatch = static_cast<uint32_t>(float(c_maxGroupDispatchCount) / groupsPerInstance);
			const uint32_t dispatchCount = DivideByMultiple(m_instanceCount, maxInstancePerBatch);

			for (uint32_t j = 0; j < dispatchCount; ++j)
			{
				const uint32_t instanceOffset = maxInstancePerBatch * j;
				const uint32_t instanceCount = min(m_instanceCount - instanceOffset, maxInstancePerBatch);

				context.SetConstant(1, 0, instanceCount);
				context.SetConstant(1, 1, instanceOffset);
				context.SetConstant(1, 2, mesh.indexSize);
				context.SetConstant(1, 3, subset.count);
				context.SetConstant(1, 4, subset.offset);

				const uint32_t groupCount = static_cast<uint32_t>(ceilf(groupsPerInstance * instanceCount));
				context.DispatchMesh(groupCount, 1, 1);
			}
		}

		++meshIndex;
	}

	RenderUI(context);

	context.EndRendering();
	context.TransitionResource(GetColorBuffer(), ResourceState::Present);

	context.Finish();
}


void MeshletInstancingApp::CreateDeviceDependentResources()
{
	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		1000.0f);
	m_camera.SetPosition(Vector3(0.0f, 5.0f, 35.0f));

	m_constantBuffer = CreateConstantBuffer("Constant Buffer", 1, sizeof(Constants));

	InitRootSignature();

	m_model.LoadFromFile("ToyRobot.bin");
	m_model.InitResources(m_deviceManager->GetDevice());

	InitDescriptorSets();

	RegenerateInstances();

	Vector3 center = m_model.GetBoundingSphere().Center;

	m_controller.SetSpeedScale(0.025f);
	m_controller.SetCameraMode(CameraMode::ArcBall);
	m_controller.RefreshFromCamera();
	m_controller.SetOrbitTarget(center, Length(m_camera.GetPosition()), 0.25f);

	UpdateConstantBuffer();
}


void MeshletInstancingApp::CreateWindowSizeDependentResources()
{
	if (!m_pipelineCreated)
	{
		InitPipeline();
		m_pipelineCreated = true;
	}

	m_camera.SetPerspectiveMatrix(
		DirectX::XMConvertToRadians(60.0f),
		GetWindowAspectRatio(),
		0.1f,
		1000.0f);
}


void MeshletInstancingApp::InitRootSignature()
{
	RootSignatureDesc desc{
		.name = "Root Signature",
		.rootParameters = {
			Table({ ConstantBuffer }, ShaderStage::Mesh | ShaderStage::Pixel),
			RootConstants(1, 5, ShaderStage::Mesh),
			Table({ StructuredBufferSRV, StructuredBufferSRV, RawBufferSRV, StructuredBufferSRV }, ShaderStage::Mesh),
			Table({ StructuredBufferSRV(4) }, ShaderStage::Mesh)
		}
	};
	m_rootSignature = CreateRootSignature(desc);
}


void MeshletInstancingApp::InitPipeline()
{
	MeshletPipelineDesc desc{
		.name				= "Meshlet Pipeline",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState	= CommonStates::RasterizerDefaultCW(),
		.rtvFormats			= { GetColorFormat()},
		.dsvFormat			= GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.meshShader			= { .shaderFile = "MeshletMS" },
		.pixelShader		= { .shaderFile = "MeshletPS" },
		.rootSignature		= m_rootSignature
	};
	m_meshletPipeline = CreateMeshletPipeline(desc);
}


void MeshletInstancingApp::InitDescriptorSets()
{
	m_cbvDescriptorSet = m_rootSignature->CreateDescriptorSet(0);
	m_cbvDescriptorSet->SetCBV(0, m_constantBuffer->GetCbvDescriptor());

	m_srvDescriptorSets.resize(m_model.GetMeshCount());
	for (uint32_t i = 0; i < (uint32_t)m_model.GetMeshCount(); ++i)
	{
		const auto& mesh = m_model.GetMesh(i);

		m_srvDescriptorSets[i] = m_rootSignature->CreateDescriptorSet(2);
		m_srvDescriptorSets[i]->SetSRV(0, mesh.vertexResources[0]->GetSrvDescriptor());
		m_srvDescriptorSets[i]->SetSRV(1, mesh.meshletResource->GetSrvDescriptor());
		m_srvDescriptorSets[i]->SetSRV(2, mesh.uniqueVertexIndexResource->GetSrvDescriptor());
		m_srvDescriptorSets[i]->SetSRV(3, mesh.primitiveIndexResource->GetSrvDescriptor());
	}


	m_instanceDescriptorSet = m_rootSignature->CreateDescriptorSet(3);
}


void MeshletInstancingApp::UpdateConstantBuffer()
{
	m_constants.viewMatrix = m_camera.GetViewMatrix();
	m_constants.viewProjectionMatrix = m_camera.GetViewProjectionMatrix();
	m_constants.drawMeshlets = 1;

	m_constantBuffer->Update(sizeof(Constants), &m_constants);
}


void MeshletInstancingApp::RegenerateInstances()
{
	const float radius = m_model.GetBoundingSphere().Radius;
	const float padding = 0.0f;
	const float spacing = (1.0f + padding) * radius * 2.0f;

	// Create the instances in a growing cube volume
	const uint32_t width = m_instanceLevel * 2 + 1;
	const float extents = spacing * m_instanceLevel;

	m_instanceCount = width * width * width;

	const size_t cbufferAlignment = m_deviceManager->GetDevice()->GetDeviceCaps().memoryAlignment.constantBufferOffset;
	const uint32_t instanceBufferSize = (uint32_t)AlignUp(m_instanceCount * sizeof(Instance), cbufferAlignment);

	m_deviceManager->WaitForGpu();

	Instance* instanceData = (Instance*)_aligned_malloc(instanceBufferSize, cbufferAlignment);
	ZeroMemory(instanceData, instanceBufferSize);

	// Regenerate the instances in our scene.
	for (uint32_t i = 0; i < m_instanceCount; ++i)
	{
		Vector3 index{ float(i % width), float((i / width) % width), float(i / (width * width)) };
		Vector3 location = spacing * index - Vector3(extents);

		Matrix4 world = Matrix4::MakeTranslation(location);

		auto& inst = instanceData[i];
		inst.worldMatrix = world;
		inst.worldInvTransposeMatrix = Invert(Transpose(world));
	}

	GpuBufferDesc desc{
		.name = "Instance Buffer",
		.resourceType = ResourceType::StructuredBuffer,
		.memoryAccess = MemoryAccess::GpuRead,
		.elementCount = m_instanceCount,
		.elementSize = instanceBufferSize / m_instanceCount,
		.initialData = instanceData
	};
	m_instanceBuffer = CreateGpuBuffer(desc);

	_aligned_free(instanceData);

	m_instanceDescriptorSet->SetSRV(0, m_instanceBuffer->GetSrvDescriptor());

}