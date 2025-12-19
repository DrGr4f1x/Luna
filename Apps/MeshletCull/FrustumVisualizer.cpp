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

#include "FrustumVisualizer.h"

#include "Application.h"
#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"
#include "Graphics\Device.h"

using namespace Luna;
using namespace Math;


FrustumVisualizer::FrustumVisualizer(Application* application)
	: m_application{ application }
{}


void FrustumVisualizer::Render(GraphicsContext& context)
{
	ScopedDrawEvent event(context, "Frustum Visualizer");

	context.SetRootSignature(m_rootSignature);
	
	// TODO: support SetConstantBuffer, either with push descriptors or DynamicDescriptorHeap
	//context.SetConstantBuffer(0, m_constantBuffer);
	context.SetDescriptors(0, m_descriptorSet);

	context.SetMeshletPipeline(m_pipeline);

	context.DispatchMesh(1, 1, 1);
}


void FrustumVisualizer::Update(Matrix4 viewProjectionMatrix, Vector4 (&planes)[6])
{
	m_constants.viewProjectionMatrix = viewProjectionMatrix;
	m_constants.lineColor = Vector4(DirectX::Colors::Purple);

	for (uint32_t i = 0; i < _countof(planes); ++i)
	{
		m_constants.planes[i] = planes[i];
	}

	m_constantBuffer->Update(sizeof(Constants), &m_constants);
}


void FrustumVisualizer::CreateDeviceDependentResources(IDevice* device)
{
	// Root signature
	{
		RootSignatureDesc desc{
			.name			= "FrustumVisualizer Root Signature",
			.rootParameters	= {	RootCBV(0, ShaderStage::Mesh) }
		};
		m_rootSignature = device->CreateRootSignature(desc);
	}

	// Constant buffer
	{
		GpuBufferDesc desc{
			.name			= "FrustumVisualizer Constant Buffer",
			.resourceType	= ResourceType::ConstantBuffer,
			.memoryAccess	= MemoryAccess::CpuWrite | MemoryAccess::GpuRead,
			.elementCount	= 1,
			.elementSize	= sizeof(Constants)
		};
		m_constantBuffer = device->CreateGpuBuffer(desc);
	}

	// Descriptor set
	{
		m_descriptorSet = m_rootSignature->CreateDescriptorSet(0);
		m_descriptorSet->SetCBV(0, m_constantBuffer);
	}
}


void FrustumVisualizer::CreateWindowSizeDependentResources(IDevice* device)
{
	// Meshlet pipeline state
	{
		MeshletPipelineDesc desc{
			.name				= "FrustumVisualizer Pipeline",
			.blendState			= CommonStates::BlendDisable(),
			.depthStencilState	= CommonStates::DepthStateDisabled(),
			.rasterizerState	= CommonStates::RasterizerTwoSided(),
			.rtvFormats			= { m_application->GetColorFormat()},
			.dsvFormat			= m_application->GetDepthFormat(),
			.topology			= PrimitiveTopology::LineList,
			.meshShader			= { .shaderFile = "FrustumMS" },
			.pixelShader		= { .shaderFile = "DebugDrawPS" },
			.rootSignature		= m_rootSignature
		};
		m_pipeline = device->CreateMeshletPipeline(desc);
	}
}