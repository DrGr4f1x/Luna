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

	m_gsContourConstantBuffer = CreateConstantBuffer("GS Contour Constant Buffer", 1, sizeof(GSContourConstants));
	m_psContourConstantBuffer = CreateConstantBuffer("PS Contour Constant Buffer", 1, sizeof(PSContourConstants));
}


void EndCapGenerator::CreateWindowSizeDependentResources()
{
	InitBuffers();

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
	ScopedDrawEvent event(context, "End Cap");

	context.TransitionResource(m_colorBuffer, ResourceState::RenderTarget);
	context.TransitionResource(m_normalBuffer, ResourceState::RenderTarget);
	context.TransitionResource(m_depthBuffer, ResourceState::DepthWrite);
	context.ClearColor(m_colorBuffer);
	context.ClearColor(m_normalBuffer);
	context.ClearDepthAndStencil(m_depthBuffer);

	uint32_t width = (uint32_t)m_colorBuffer->GetWidth();
	uint32_t height = m_colorBuffer->GetHeight();

	context.BeginRendering({ m_colorBuffer, m_normalBuffer }, m_depthBuffer);
	context.SetViewportAndScissor(0u, 0u, width, height);

	context.SetStencilRef(0x1);

	// Draw the contour
	context.SetRootSignature(m_contourRootSignature);
	context.SetGraphicsPipeline(m_contourPipeline);

	context.SetRootCBV(0, m_gsContourConstantBuffer);
	context.SetRootCBV(1, m_psContourConstantBuffer);

	model->Render(context);

	context.EndRendering();
}


void EndCapGenerator::InitRootSignatures()
{
	RootSignatureDesc contourDesc{
		.name				= "Contour Root Signature",
		.rootParameters		= {	
			RootCBV(0, ShaderStage::Geometry),
			RootCBV(0, ShaderStage::Pixel)
	}
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
	RasterizerStateDesc rasterizerDesc = CommonStates::RasterizerDefault();
	rasterizerDesc.multisampleEnable = true;

	DepthStencilStateDesc depthStencilDesc = CommonStates::DepthStateReadWriteReversed();
	depthStencilDesc.depthFunc = ComparisonFunc::Always;
	depthStencilDesc.stencilEnable = true;
	depthStencilDesc.backFace.stencilFunc = ComparisonFunc::Always;
	depthStencilDesc.backFace.stencilDepthFailOp = StencilOp::Replace;
	depthStencilDesc.backFace.stencilFailOp = StencilOp::Replace;
	depthStencilDesc.backFace.stencilPassOp = StencilOp::Replace;
	depthStencilDesc.frontFace = depthStencilDesc.backFace;

	GraphicsPipelineDesc contourPipelineDesc{
		.name				= "Contour Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= depthStencilDesc,
		.rasterizerState	= rasterizerDesc,
		.rtvFormats			= { m_colorBuffer->GetFormat(), m_normalBuffer->GetFormat() },
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


void EndCapGenerator::InitBuffers()
{
	uint32_t width = m_app->GetWindowWidth();
	uint32_t height = m_app->GetWindowHeight();

	ColorBufferDesc colorBufferDesc{
		.name	= "Contour color buffer",
		.width	= width,
		.height = height,
		.format = Format::RGBA8_UNorm
	};
	m_colorBuffer = m_app->CreateColorBuffer(colorBufferDesc);

	ColorBufferDesc normalBufferDesc{
		.name	= "Contour normal buffer",
		.width	= width,
		.height = height,
		.format = Format::RGBA16_Float
	};
	m_normalBuffer = m_app->CreateColorBuffer(normalBufferDesc);

	DepthBufferDesc depthBufferDesc{
		.name = "Contour depth buffer",
		.width = width,
		.height = height,
		.format = m_app->GetDepthFormat()
	};
	m_depthBuffer = m_app->CreateDepthBuffer(depthBufferDesc);
}


void EndCapGenerator::UpdateConstantBuffers(float planeY)
{
	Matrix4 modelMatrix{ kIdentity };

	m_gsContourConstants.modelViewProjectionMatrix = m_app->GetCamera()->GetViewProjectionMatrix() * modelMatrix;
	m_gsContourConstants.modelViewMatrix = m_app->GetCamera()->GetViewMatrix() * modelMatrix;
	m_gsContourConstants.modelMatrix = modelMatrix;

	Vector3 planeNormal = Vector3(0.0f, 1.0f, 0.0f);
	Vector3 pointOnPlane = Vector3(0.0f, planeY, 0.0f);
	m_gsContourConstants.plane = Vector4(planeNormal, -Dot(pointOnPlane, planeNormal));

	m_gsContourConstantBuffer->Update(sizeof(GSContourConstants), &m_gsContourConstants);

	m_psContourConstants.modelViewMatrix = m_gsContourConstants.modelViewMatrix;
	m_psContourConstants.viewPos = Vector4(m_app->GetCamera()->GetPosition(), 0.0);

	m_psContourConstantBuffer->Update(sizeof(PSContourConstants), &m_psContourConstants);
}