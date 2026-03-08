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

#include "EndCapGenerator.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"
#include "Graphics\InputLayout.h"


using namespace Luna;
using namespace Math;


EndCapGenerator::EndCapGenerator(Application* app)
	: m_app{ app }
{}


void EndCapGenerator::CreateDeviceDependentResources()
{
	InitRootSignatures();

	m_contourConstantBuffer = CreateConstantBuffer("Contour Constant Buffer", 1, sizeof(ContourConstants));
}


void EndCapGenerator::CreateWindowSizeDependentResources()
{
	if (!m_pipelineCreated)
	{
		InitPipelines();
	}
}


void EndCapGenerator::Update(float planeY)
{
	UpdateConstantBuffers(planeY);
}


void EndCapGenerator::Render(GraphicsContext& context, Model* model)
{
	// Draw the contour
	context.SetRootSignature(m_contourRootSignature);
	context.SetGraphicsPipeline(m_contourPipeline);

	context.SetRootCBV(0, m_contourConstantBuffer);

	model->Render(context);
}


void EndCapGenerator::InitRootSignatures()
{
	RootSignatureDesc contourDesc{
		.name				= "Contour Root Signature",
		.rootParameters		= {	RootCBV(0, ShaderStage::Geometry) }
	};

	m_contourRootSignature = m_app->CreateRootSignature(contourDesc);
}


void EndCapGenerator::InitPipelines()
{
	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= sizeof(Vertex),
		.inputClassification	= InputClassification::PerVertexData
	};

	auto layout = VertexLayout<VertexComponent::PositionNormalColor>();

	// Contour pipeline
	GraphicsPipelineDesc contourPipelineDesc{
		.name				= "Contour Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateDisabled(),
		.rasterizerState	= CommonStates::RasterizerDefault(),
		.rtvFormats			= { m_app->GetColorFormat() },
		.dsvFormat			= m_app->GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "ContourVS" },
		.pixelShader		= { .shaderFile = "ContourPS" },
		.geometryShader		= { .shaderFile = "ContourGS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= layout.GetElements(),
		.rootSignature		= m_contourRootSignature
	};

	m_contourPipeline = m_app->CreateGraphicsPipeline(contourPipelineDesc);
}


void EndCapGenerator::UpdateConstantBuffers(float planeY)
{
	m_contourConstants.viewProjectionMatrix = m_app->GetCamera()->GetViewProjectionMatrix();
	m_contourConstants.modelMatrix = Matrix4(kIdentity);

	Vector3 planeNormal = Vector3(0.0f, 1.0f, 0.0f);
	Vector3 pointOnPlane = Vector3(0.0f, planeY, 0.0f);
	m_contourConstants.plane = Vector4(planeNormal, -Dot(pointOnPlane, planeNormal));

	m_contourConstantBuffer->Update(sizeof(ContourConstants), &m_contourConstants);
}