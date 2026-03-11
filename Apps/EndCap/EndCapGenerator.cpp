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
	m_jumpFloodConstantBuffer = CreateConstantBuffer("Jump flood constant buffer", 1, sizeof(JumpFloodConstants));
	m_medianFilterConstantBuffer = CreateConstantBuffer("Median filter constant buffer", 1, sizeof(MedianFilterConstants));
}


void EndCapGenerator::CreateWindowSizeDependentResources()
{
	InitBuffers();

	InitJfaSteps();

	InitDescriptorSets();

	if (!m_pipelineCreated)
	{
		InitPipelines();
	}
}


void EndCapGenerator::Update(float planeY)
{
	UpdateConstantBuffers(planeY);
}


void EndCapGenerator::Render(GraphicsContext& context, Model* model, bool multipleModels)
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

	context.SetRootCBV(1, m_gsContourConstantBuffer);
	context.SetRootCBV(2, m_psContourConstantBuffer);

	if (multipleModels)
	{
		float xOffset[] = { -0.6f, 0.0f, 0.6f };
		for (uint32_t i = 0; i < _countof(xOffset); ++i)
		{
			context.SetConstants(0, xOffset[i], 0.0f, 0.0f);
			model->Render(context);
		}
	}
	else
	{
		context.SetConstants(0, 0.0f, 0.0f, 0.0f);
		model->Render(context);
	}

	context.EndRendering();

	// Jump flood init
	auto& computeContext = context.GetComputeContext();
	{
		ScopedDrawEvent event(computeContext, "Jump Flood Init");

		context.TransitionResource(m_jumpFloodDataBuffers[0], ResourceState::RenderTarget);
		context.TransitionResource(m_jumpFloodDataBuffers[1], ResourceState::RenderTarget);
		context.TransitionResource(m_jumpFloodClassBuffers[0], ResourceState::RenderTarget);
		context.TransitionResource(m_jumpFloodClassBuffers[1], ResourceState::RenderTarget);

		context.ClearColor(m_jumpFloodDataBuffers[0]);
		context.ClearColor(m_jumpFloodDataBuffers[1]);
		context.ClearColor(m_jumpFloodClassBuffers[0]);
		context.ClearColor(m_jumpFloodClassBuffers[1]);

		computeContext.TransitionResource(m_colorBuffer, ResourceState::NonPixelShaderResource);
		computeContext.TransitionResource(m_normalBuffer, ResourceState::NonPixelShaderResource);
		computeContext.TransitionResource(m_jumpFloodDataBuffers[0], ResourceState::UnorderedAccess);
		computeContext.TransitionResource(m_jumpFloodClassBuffers[0], ResourceState::UnorderedAccess);

		computeContext.SetRootSignature(m_jumpFloodInitRootSig);
		computeContext.SetComputePipeline(m_jumpFloodInitPipeline);

		computeContext.SetDescriptors(0, m_jumpFloodInitDescriptors);

		computeContext.Dispatch2D(width, height, 8, 8);
	}

	// Jump flood algorithm
	uint32_t readIndex = 0;
	uint32_t writeIndex = 1;
	uint32_t passIndex = 0;

	computeContext.SetRootSignature(m_jumpFloodRootSig);
	computeContext.SetComputePipeline(m_jumpFloodPipeline);

	ScopedDrawEvent event2(computeContext, "Jump Flood");

	for (uint32_t i = 0; i < (uint32_t)m_jfaSteps.size(); ++i)
	{
		computeContext.TransitionResource(m_jumpFloodDataBuffers[readIndex], ResourceState::NonPixelShaderResource);
		computeContext.TransitionResource(m_jumpFloodClassBuffers[readIndex], ResourceState::NonPixelShaderResource);
		computeContext.TransitionResource(m_jumpFloodDataBuffers[writeIndex], ResourceState::UnorderedAccess);
		computeContext.TransitionResource(m_jumpFloodClassBuffers[writeIndex], ResourceState::UnorderedAccess);

		computeContext.SetDescriptors(0, m_jumpFloodDescriptors[i]);

		uint32_t stepX = m_jfaSteps[i].first;
		uint32_t stepY = m_jfaSteps[i].second;
		computeContext.SetConstants(1, stepX, stepY);

		computeContext.Dispatch2D(width, height, 8, 8);

		std::swap(readIndex, writeIndex);
	}

	// Median filter
	{
		computeContext.TransitionResource(m_jumpFloodClassBuffers[writeIndex], ResourceState::NonPixelShaderResource);
		computeContext.TransitionResource(m_filteredClassBuffer, ResourceState::UnorderedAccess);

		computeContext.SetRootSignature(m_medianFilterRootSig);
		computeContext.SetComputePipeline(m_medianFilterPipeline);

		ScopedDrawEvent event2(computeContext, "Median Filter");

		computeContext.SetDescriptors(0, m_medianFilterDescriptors[writeIndex]);

		computeContext.Dispatch2D(width, height, 8, 8);
	}
}


void EndCapGenerator::InitRootSignatures()
{
	RootSignatureDesc contourDesc{
		.name				= "Contour Root Signature",
		.rootParameters		= {
			RootConstants(0, 3, ShaderStage::Vertex),
			RootCBV(0, ShaderStage::Geometry),
			RootCBV(0, ShaderStage::Pixel)
		}
	};

	m_contourRootSignature = m_app->CreateRootSignature(contourDesc);

	RootSignatureDesc jumpFloodInitDesc{
		.name = "Jump flood init RS",
		.rootParameters = {
			Table({TextureSRV, TextureSRV, TextureUAV, TextureUAV}, ShaderStage::Compute)
		}
	};

	m_jumpFloodInitRootSig = m_app->CreateRootSignature(jumpFloodInitDesc);

	RootSignatureDesc jumpFloodDesc{
		.name = "Jump flood RS",
		.rootParameters = {
			Table({TextureSRV, TextureSRV, TextureUAV, TextureUAV, ConstantBuffer}, ShaderStage::Compute),
			RootConstants(1, 2, ShaderStage::Compute)
		}
	};

	m_jumpFloodRootSig = m_app->CreateRootSignature(jumpFloodDesc);

	RootSignatureDesc medianFilterDesc{
		.name = "Median filter RS",
		.rootParameters = {
			Table({TextureSRV, TextureSRV, TextureUAV, ConstantBuffer}, ShaderStage::Compute)
		},
		.staticSamplers = { StaticSampler(CommonStates::SamplerLinearClamp()) }
	};

	m_medianFilterRootSig = m_app->CreateRootSignature(medianFilterDesc);
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

	// Jump flood init
	ComputePipelineDesc jumpFloodInitDesc{
		.name			= "Jump flood init PSO",
		.computeShader	= {.shaderFile = "JumpFloodInitCS" },
		.rootSignature	= m_jumpFloodInitRootSig
	};
	m_jumpFloodInitPipeline = m_app->CreateComputePipeline(jumpFloodInitDesc);

	// Jump flood
	ComputePipelineDesc jumpFloodDesc{
		.name			= "Jump flood PSO",
		.computeShader	= { .shaderFile = "JumpFloodCS" },
		.rootSignature	= m_jumpFloodRootSig
	};
	m_jumpFloodPipeline = m_app->CreateComputePipeline(jumpFloodDesc);

	// Median filter
	ComputePipelineDesc medianFilterDesc{
		.name			= "Median filter PSO",
		.computeShader	= { .shaderFile = "MedianFilterCS" },
		.rootSignature	= m_medianFilterRootSig
	};
	m_medianFilterPipeline = m_app->CreateComputePipeline(medianFilterDesc);
}


void EndCapGenerator::InitBuffers()
{
	uint32_t width = m_app->GetWindowWidth();
	uint32_t height = m_app->GetWindowHeight();

	// Contour buffers
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
		.name	= "Contour depth buffer",
		.width	= width,
		.height = height,
		.format = m_app->GetDepthFormat()
	};
	m_depthBuffer = m_app->CreateDepthBuffer(depthBufferDesc);

	// Jump flood buffers
	for (uint32_t i = 0; i < 2; ++i)
	{
		ColorBufferDesc dataDesc{
			.name	= std::format("Jump flood color buffer {}", i),
			.width	= width,
			.height = height,
			.format = Format::RGBA16_Float
		};
		m_jumpFloodDataBuffers[i] = m_app->CreateColorBuffer(dataDesc);

		ColorBufferDesc classDesc{
			.name	= std::format("Jump flood class buffer {}", i),
			.width	= width,
			.height = height,
			.format = Format::R8_UInt
		};
		m_jumpFloodClassBuffers[i] = m_app->CreateColorBuffer(classDesc);
	}

	// Median filter buffer
	ColorBufferDesc filteredBufferDesc{
		.name	= "Filtered class buffer",
		.width	= width,
		.height	= height,
		.format = Format::R8_UInt
	};
	m_filteredClassBuffer = m_app->CreateColorBuffer(filteredBufferDesc);
}


void EndCapGenerator::InitJfaSteps()
{
	const uint32_t width = (uint32_t)m_colorBuffer->GetWidth();
	const uint32_t height = m_colorBuffer->GetHeight();
	uint32_t stepX = width;
	uint32_t stepY = height;

	m_jfaSteps.clear();

	// 1-JFA
	//m_jfaSteps.push_back(std::make_pair(1, 1));

	// Main JFA
	while (stepX > 1 || stepY > 1)
	{
		stepX >>= 1;
		if (stepX == 0)
		{
			stepX = 1;
		}
		stepY >>= 1;
		if (stepY == 0)
		{
			stepY = 1;
		}

		m_jfaSteps.push_back(std::make_pair(stepX, stepY));
	}

	// JFA-2
	m_jfaSteps.push_back(std::make_pair(2, 2));
	m_jfaSteps.push_back(std::make_pair(1, 1));

	// JFA-1
	//m_jfaSteps.push_back(std::make_pair(1, 1));
}


void EndCapGenerator::InitDescriptorSets()
{
	// Jump flood init
	m_jumpFloodInitDescriptors = m_jumpFloodInitRootSig->CreateDescriptorSet(0);
	m_jumpFloodInitDescriptors->SetSRV(0, m_colorBuffer);
	m_jumpFloodInitDescriptors->SetSRV(1, m_normalBuffer);
	m_jumpFloodInitDescriptors->SetUAV(0, m_jumpFloodDataBuffers[0]);
	m_jumpFloodInitDescriptors->SetUAV(1, m_jumpFloodClassBuffers[0]);

	// Jump flood
	uint32_t readIndex = 0;
	uint32_t writeIndex = 1;

	m_jumpFloodDescriptors.clear();

	for (uint32_t i = 0; i < (uint32_t)m_jfaSteps.size(); ++i)
	{
		auto descriptorSet = m_jumpFloodRootSig->CreateDescriptorSet(0);
		descriptorSet->SetSRV(0, m_jumpFloodDataBuffers[readIndex]);
		descriptorSet->SetSRV(1, m_jumpFloodClassBuffers[readIndex]);
		descriptorSet->SetUAV(0, m_jumpFloodDataBuffers[writeIndex]);
		descriptorSet->SetUAV(1, m_jumpFloodClassBuffers[writeIndex]);
		descriptorSet->SetCBV(0, m_jumpFloodConstantBuffer);

		m_jumpFloodDescriptors.push_back(descriptorSet);

		std::swap(readIndex, writeIndex);
	}

	 // Median filter
	m_medianFilterDescriptors[0] = m_medianFilterRootSig->CreateDescriptorSet(0);
	m_medianFilterDescriptors[0]->SetSRV(0, m_jumpFloodClassBuffers[0]);
	m_medianFilterDescriptors[0]->SetSRV(1, m_colorBuffer);
	m_medianFilterDescriptors[0]->SetUAV(0, m_filteredClassBuffer);
	m_medianFilterDescriptors[0]->SetCBV(0, m_medianFilterConstantBuffer);

	m_medianFilterDescriptors[1] = m_medianFilterRootSig->CreateDescriptorSet(0);
	m_medianFilterDescriptors[1]->SetSRV(0, m_jumpFloodClassBuffers[1]);
	m_medianFilterDescriptors[1]->SetSRV(1, m_colorBuffer);
	m_medianFilterDescriptors[1]->SetUAV(0, m_filteredClassBuffer);
	m_medianFilterDescriptors[1]->SetCBV(0, m_medianFilterConstantBuffer);
}


void EndCapGenerator::UpdateConstantBuffers(float planeY)
{
	Matrix4 modelMatrix{ kIdentity };

	// Contours
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

	// Jump flood
	m_jumpFloodConstants.texWidth = (int)m_colorBuffer->GetWidth();
	m_jumpFloodConstants.texHeight = (int)m_colorBuffer->GetHeight();
	m_jumpFloodConstantBuffer->Update(sizeof(JumpFloodConstants), &m_jumpFloodConstants);

	// Median filter
	m_medianFilterConstants.texDimensions[0] = (float)m_colorBuffer->GetWidth();
	m_medianFilterConstants.texDimensions[1] = (float)m_colorBuffer->GetHeight();
	m_medianFilterConstants.invTexDimensions[0] = 1.0f / m_medianFilterConstants.texDimensions[0];
	m_medianFilterConstants.invTexDimensions[1] = 1.0f / m_medianFilterConstants.texDimensions[1];
	m_medianFilterConstantBuffer->Update(sizeof(MedianFilterConstants), &m_medianFilterConstants);
}