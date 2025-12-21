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

#include "CullDataVisualizer.h"

#include "Application.h"
#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"
#include "Graphics\Device.h"

using namespace Luna;
using namespace Math;
using namespace DirectX;


CullDataVisualizer::CullDataVisualizer(Application* application)
	: m_application{ application }
{}


void CullDataVisualizer::Render(GraphicsContext& context, const MeshletMesh& mesh, uint32_t offset, uint32_t count)
{
	// Push constant data to GPU for our shader invocations
	assert(offset + count <= static_cast<uint32_t>(mesh.meshlets.size()));

	// Shared root signature between two shaders
	context.SetRootSignature(m_rootSignature);
	
	context.SetRootCBV(0, m_constantBuffer);
	context.SetConstants(1, offset, count);
	context.SetRootSRV(2, mesh.cullDataResource);

	// Dispatch bounding sphere draw
	context.SetMeshletPipeline(m_boundingSpherePipeline);
	context.DispatchMesh(count, 1, 1);

	// Dispatch normal code draw
	context.SetMeshletPipeline(m_normalConePipeline);
	context.DispatchMesh(count, 1, 1);
}


void CullDataVisualizer::Update(Math::Matrix4 worldMatrix, Math::Matrix4 viewMatrix, Math::Matrix4 projectionMatrix, Math::Vector4 color)
{
	XMVECTOR scale, rotation, position;
	XMMatrixDecompose(&scale, &rotation, &position, worldMatrix); // Need a transpose here?

	Matrix4 viewToWorldMatrix = Transpose(viewMatrix);
	XMVECTOR camUp = XMVector3TransformNormal(g_XMIdentityR1, viewToWorldMatrix);
	XMVECTOR camForward = XMVector3TransformNormal(g_XMNegIdentityR2, viewToWorldMatrix);

	m_constants.viewProjectionMatrix = projectionMatrix * viewMatrix;
	m_constants.worldMatrix = worldMatrix;
	m_constants.color = color;
	m_constants.viewUpVector = Vector4(camUp);
	m_constants.viewForwardVector = Vector4(camForward);
	m_constants.scale = Vector3(scale).GetX();
	m_constants.color.SetW(0.3f);

	m_constantBuffer->Update(sizeof(Constants), &m_constants);
}


void CullDataVisualizer::CreateDeviceDependentResources(IDevice* device)
{
	// Root signature
	{
		RootSignatureDesc desc{
			.name				= "CullDataVisualizer Root Signature",
			.rootParameters		= {
				RootCBV(0, ShaderStage::Mesh),
				RootConstants(1, 2, ShaderStage::Mesh),
				RootSRV(1, ShaderStage::Mesh)
			}
		};
		m_rootSignature = device->CreateRootSignature(desc);
	}

	// Constant buffer
	{
		GpuBufferDesc desc{
			.name			= "CullDataVisualizer Constant Buffer",
			.resourceType	= ResourceType::ConstantBuffer,
			.memoryAccess	= MemoryAccess::CpuWrite | MemoryAccess::GpuRead,
			.elementCount	= 1,
			.elementSize	= sizeof(Constants)
		};
		m_constantBuffer = device->CreateGpuBuffer(desc);
	}
}


void CullDataVisualizer::CreateWindowSizeDependentResources(IDevice* device)
{
	// BoundingSphere pipeline state
	{
		MeshletPipelineDesc desc{
			.name				= "BoundingSphere Pipeline",
			.blendState			= CommonStates::BlendDisable(),
			.depthStencilState	= CommonStates::DepthStateDisabled(),
			.rasterizerState	= CommonStates::RasterizerTwoSided(),
			.rtvFormats			= { m_application->GetColorFormat()},
			.dsvFormat			= m_application->GetDepthFormat(),
			.topology			= PrimitiveTopology::TriangleList,
			.meshShader			= { .shaderFile = "BoundingSphereMS" },
			.pixelShader		= { .shaderFile = "DebugDrawPS" },
			.rootSignature		= m_rootSignature
		};
		m_boundingSpherePipeline = device->CreateMeshletPipeline(desc);
	}

	// NormalCone pipeline state
	{
		MeshletPipelineDesc desc{
			.name				= "NormalCone Pipeline",
			.blendState			= CommonStates::BlendDisable(),
			.depthStencilState	= CommonStates::DepthStateDisabled(),
			.rasterizerState	= CommonStates::RasterizerTwoSided(),
			.rtvFormats			= { m_application->GetColorFormat()},
			.dsvFormat			= m_application->GetDepthFormat(),
			.topology			= PrimitiveTopology::LineList,
			.meshShader			= { .shaderFile = "NormalConeMS" },
			.pixelShader		= { .shaderFile = "DebugDrawPS" },
			.rootSignature		= m_rootSignature
		};
		m_normalConePipeline = device->CreateMeshletPipeline(desc);
	}
}