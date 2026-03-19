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
	m_edgeCrossingConstantBuffer = CreateConstantBuffer("Edge crossing constant buffer", 1, sizeof(EdgeCrossingConstants));
	m_medianFilterConstantBuffer = CreateConstantBuffer("Median filter constant buffer", 1, sizeof(MedianFilterConstants));
	m_gsDebugNormalsConstantBuffer = CreateConstantBuffer("GS Debug Normals Constant Buffer", 1, sizeof(GSDebugNormalsConstants));
}


void EndCapGenerator::CreateWindowSizeDependentResources()
{
	InitBuffers();

	InitDescriptorSets();

	if (!m_pipelineCreated)
	{
		InitPipelines();
	}
}


void EndCapGenerator::Update(bool debugNormals, float planeY, float normalLength)
{
	m_debugNormals = debugNormals;
	UpdateConstantBuffers(planeY, normalLength);
}


void EndCapGenerator::Render(GraphicsContext& context, Model* model, bool multipleModels)
{
	ScopedDrawEvent event(context, "End Cap");

	uint32_t width = (uint32_t)m_contourDataBuffer->GetWidth();
	uint32_t height = m_contourDataBuffer->GetHeight();

	auto& computeContext = context.GetComputeContext();

	// Init bounds
	{
		ScopedDrawEvent event2(context, "Init Bounds");

		computeContext.TransitionResource(m_boundsBuffer, ResourceState::UnorderedAccess);

		computeContext.SetRootSignature(m_boundsInitRootSig);
		computeContext.SetComputePipeline(m_boundsInitPipeline);

		computeContext.SetDescriptors(0, m_boundsInitDescriptors);
		computeContext.SetConstants(1, (int32_t)width, (int32_t)height);
		computeContext.Dispatch1D(64);
	}

	// Init border
	{
		ScopedDrawEvent event2(context, "Init Border");

		computeContext.TransitionResource(m_leftBorderBuffer, ResourceState::UnorderedAccess);
		computeContext.TransitionResource(m_rightBorderBuffer, ResourceState::UnorderedAccess);
		computeContext.TransitionResource(m_topBorderBuffer, ResourceState::UnorderedAccess);
		computeContext.TransitionResource(m_bottomBorderBuffer, ResourceState::UnorderedAccess);

		computeContext.SetRootSignature(m_borderInitRootSig);
		computeContext.SetComputePipeline(m_borderInitPipeline);

		// Left-right borders
		computeContext.SetDescriptors(0, m_leftRightBorderInitDescriptors);
		computeContext.SetConstants(1, height, width);

		computeContext.Dispatch1D(height, 64);

		// Top-bottom borders
		computeContext.SetDescriptors(0, m_topBottomBorderInitDescriptors);
		computeContext.SetConstants(1, width, height);

		computeContext.Dispatch1D(width, 64);
	}

	{
		ScopedDrawEvent event2(context, "Draw Contours");
		context.TransitionResource(m_contourDataBuffer, ResourceState::RenderTarget);
		context.TransitionResource(m_depthBuffer, ResourceState::DepthWrite);
		context.TransitionResource(m_outerBoundaryBuffer, ResourceState::RenderTarget);
		context.TransitionResource(m_edgeCrossingBuffer, ResourceState::RenderTarget);
		context.TransitionResource(m_edgeIdBuffer, ResourceState::RenderTarget);
		context.TransitionResource(m_edgeCrossingBuffer2, ResourceState::RenderTarget);
		context.TransitionResource(m_endCapMaskBuffer, ResourceState::RenderTarget);
		context.TransitionResource(m_fillDebugTex, ResourceState::RenderTarget);
		context.ClearColor(m_contourDataBuffer);
		context.ClearColor(m_outerBoundaryBuffer);
		context.ClearColor(m_edgeCrossingBuffer);
		context.ClearColor(m_edgeIdBuffer);
		context.ClearColor(m_edgeCrossingBuffer2);
		context.ClearColor(m_endCapMaskBuffer);
		context.ClearColor(m_fillDebugTex);

		context.ClearDepthAndStencil(m_depthBuffer);

		context.BeginRendering(m_contourDataBuffer, m_depthBuffer);
		context.SetViewportAndScissor(0u, 0u, width, height);

		// Draw the contour
		context.SetRootSignature(m_contourRootSignature);
		context.SetGraphicsPipeline(m_contourPipeline);

		context.SetRootCBV(1, m_gsContourConstantBuffer);
		context.SetDescriptors(2, m_psContourDescriptors);

		context.SetStencilRef(0x1);

		if (multipleModels)
		{
			float xOffset[] = { -0.6f, 0.0f, 0.6f };
			for (uint32_t i = 0; i < _countof(xOffset); ++i)
			{
				const float id = (float)i + 1.0f;
				context.SetConstants(0, xOffset[i], 0.0f, 0.0f, id);
				model->Render(context);
			}
		}
		else
		{
			const float id = 1.0f;
			context.SetConstants(0, 0.0f, 0.0f, 0.0f, id);
			model->Render(context);
		}

		context.EndRendering();
	}

	// Outer boundary
	{
		ScopedDrawEvent event2(computeContext, "Outer Boundary");

		computeContext.TransitionResource(m_outerBoundaryBuffer, ResourceState::UnorderedAccess);
		computeContext.TransitionResource(m_leftBorderBuffer, ResourceState::NonPixelShaderResource);
		computeContext.TransitionResource(m_rightBorderBuffer, ResourceState::NonPixelShaderResource);
		computeContext.TransitionResource(m_topBorderBuffer, ResourceState::NonPixelShaderResource);
		computeContext.TransitionResource(m_bottomBorderBuffer, ResourceState::NonPixelShaderResource);

		computeContext.SetRootSignature(m_outerBoundaryRootSig);
		computeContext.SetComputePipeline(m_outerBoundaryPipeline);

		computeContext.SetDescriptors(0, m_outerBoundaryDescriptors);
		computeContext.SetConstants(1, width, height);

		computeContext.Dispatch2D(width, height, 8, 8);
	}

	// Edge crossing
	{
		ScopedDrawEvent event2(computeContext, "Edge Crossing");

		context.TransitionResource(m_contourDataBuffer, ResourceState::NonPixelShaderResource);
		context.TransitionResource(m_edgeCrossingBuffer, ResourceState::UnorderedAccess);
		context.TransitionResource(m_edgeIdBuffer, ResourceState::UnorderedAccess);
		context.TransitionResource(m_edgeCrossingBuffer2, ResourceState::UnorderedAccess);

		computeContext.SetRootSignature(m_edgeCrossingRootSig);
		computeContext.SetComputePipeline(m_edgeCrossingPipeline);

		computeContext.SetDescriptors(0, m_edgeCrossingDescriptors);

		computeContext.Dispatch2D(width, height, 8, 8);
	}

	// Fill
	{
		// Horizontal
		{
			ScopedDrawEvent event2(computeContext, "Fill - horizontal");

			computeContext.TransitionResource(m_endCapMaskBuffer, ResourceState::UnorderedAccess);
			computeContext.TransitionResource(m_fillDebugTex, ResourceState::UnorderedAccess);
			computeContext.TransitionResource(m_edgeCrossingBuffer2, ResourceState::NonPixelShaderResource);
			computeContext.TransitionResource(m_depthBuffer, ResourceState::NonPixelShaderResource);

			computeContext.SetRootSignature(m_fillRootSig);
			computeContext.SetComputePipeline(m_fillPipeline);

			computeContext.SetDescriptors(0, m_fillDescriptors);

			computeContext.SetConstants(1, width, height, 0);

			computeContext.Dispatch1D(height, 64);

			computeContext.InsertUAVBarrier(m_endCapMaskBuffer);
			computeContext.InsertUAVBarrier(m_fillDebugTex);
		}

		// Vertical
		{
			ScopedDrawEvent event2(computeContext, "Fill - vertical");

			computeContext.SetConstants(1, width, height, 1);

			computeContext.Dispatch1D(width, 64);
		}

	}

	// Median filter
	{
		computeContext.TransitionResource(m_endCapMaskBuffer, ResourceState::NonPixelShaderResource);
		computeContext.TransitionResource(m_filteredBuffer, ResourceState::UnorderedAccess);

		computeContext.SetRootSignature(m_medianFilterRootSig);
		computeContext.SetComputePipeline(m_medianFilterPipeline);

		ScopedDrawEvent event2(computeContext, "Median Filter");

		computeContext.SetDescriptors(0, m_medianFilterDescriptors);

		computeContext.Dispatch2D(width, height, 8, 8);
	}

	// Debug normals
	if (m_debugNormals)
	{
		ScopedDrawEvent event2(context, "Debug Normals");

		context.BeginRendering(m_app->GetColorBuffer(), m_app->GetDepthBuffer());

		context.SetViewportAndScissor(0u, 0u, m_app->GetWindowWidth(), m_app->GetWindowHeight());

		// Setup for mesh drawing
		context.SetRootSignature(m_debugNormalsRootSig);
		context.SetGraphicsPipeline(m_debugNormalsPipeline);

		context.SetRootCBV(1, m_gsDebugNormalsConstantBuffer);

		if (multipleModels)
		{
			float xOffset[] = { -0.6f, 0.0f, 0.6f };
			for (uint32_t i = 0; i < _countof(xOffset); ++i)
			{
				const float id = (float)i + 1.0f;
				context.SetConstants(0, xOffset[i], 0.0f, 0.0f, id);
				model->Render(context);
			}
		}
		else
		{
			const float id = 1.0f;
			context.SetConstants(0, 0.0f, 0.0f, 0.0f, id);
			model->Render(context);
		}

		context.EndRendering();
	}
}


void EndCapGenerator::InitRootSignatures()
{
	// Bounds buffer init
	RootSignatureDesc boundsInitDesc{
		.name				= "Bounds buffer init RS",
		.rootParameters		= {
			Table({ TypedBufferUAV }, ShaderStage::Compute),
			RootConstants(0, 2, ShaderStage::Compute)
		}
	};
	m_boundsInitRootSig = m_app->CreateRootSignature(boundsInitDesc);

	// Border buffer init
	RootSignatureDesc borderInitDesc{
		.name				= "Bounds buffer init RS",
		.rootParameters		= {
			Table({ TypedBufferUAV(0, 2) }, ShaderStage::Compute),
			RootConstants(0, 2, ShaderStage::Compute)
		}
	};
	m_borderInitRootSig = m_app->CreateRootSignature(borderInitDesc);

	// Contour rendering
	RootSignatureDesc contourDesc{
		.name				= "Contour Root Signature",
		.rootParameters		= {
			RootConstants(0, 4, ShaderStage::Vertex),
			RootCBV(0, ShaderStage::Geometry),
			Table({ TypedBufferUAV(0, 5) }, ShaderStage::Pixel)
		}
	};
	m_contourRootSignature = m_app->CreateRootSignature(contourDesc);

	// Outer boundary init
	RootSignatureDesc outerBoundaryDesc{
		.name				= "Outer boundary init RS",
		.rootParameters		= {
			Table({ TextureUAV, TypedBufferSRV(0, 4) }, ShaderStage::Compute),
			RootConstants(0, 2, ShaderStage::Compute)
		}
	};
	m_outerBoundaryRootSig = m_app->CreateRootSignature(outerBoundaryDesc);

	// Edge crossing
	RootSignatureDesc edgeCrossingDesc{
		.name				= "Edge crossing RS",
		.rootParameters		= {
			Table({TextureSRV, TextureUAV(0, 3), ConstantBuffer}, ShaderStage::Compute)
		},
		.staticSamplers = { StaticSampler(CommonStates::SamplerLinearClamp()) }
	};
	m_edgeCrossingRootSig = m_app->CreateRootSignature(edgeCrossingDesc);

	// Fill 
	RootSignatureDesc fillDesc{
		.name				= "Fill RS",
		.rootParameters		= {
			Table({TextureUAV(0, 2), TextureSRV(0, 2), TypedBufferSRV(2, 4)}, ShaderStage::Compute),
			RootConstants(0, 3, ShaderStage::Compute)
		}
	};
	m_fillRootSig = m_app->CreateRootSignature(fillDesc);

	// Median filter
	RootSignatureDesc medianFilterDesc{
		.name				= "Median filter RS",
		.rootParameters		= {
			Table({TextureSRV, TextureSRV, TextureUAV, ConstantBuffer}, ShaderStage::Compute)
		},
		.staticSamplers = { StaticSampler(CommonStates::SamplerLinearClamp()) }
	};

	m_medianFilterRootSig = m_app->CreateRootSignature(medianFilterDesc);

	// Debug normals
	RootSignatureDesc debugNormalsDesc{
		.name				= "Debug Normals Root Signature",
		.rootParameters		= {
			RootConstants(0, 4, ShaderStage::Vertex),
			RootCBV(0, ShaderStage::Geometry)
		}
	};
	m_debugNormalsRootSig = m_app->CreateRootSignature(debugNormalsDesc);
}


void EndCapGenerator::InitPipelines()
{
	// Bounds init
	ComputePipelineDesc boundsInitDesc{
		.name			= "Bounds buffer init PSO",
		.computeShader	= {.shaderFile = "BoundsInitCS" },
		.rootSignature	= m_boundsInitRootSig
	};
	m_boundsInitPipeline = m_app->CreateComputePipeline(boundsInitDesc);

	// Border init
	ComputePipelineDesc borderInitDesc{
		.name			= "Border buffer init PSO",
		.computeShader	= { .shaderFile = "BorderInitCS" },
		.rootSignature	= m_borderInitRootSig
	};
	m_borderInitPipeline = m_app->CreateComputePipeline(borderInitDesc);

	// Contour pipeline
	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= sizeof(Vertex),
		.inputClassification	= InputClassification::PerVertexData
	};

	auto layout = VertexLayout<VertexComponent::PositionNormalColor>();

	RasterizerStateDesc rasterizerDesc = CommonStates::RasterizerDefault();
	rasterizerDesc.multisampleEnable = true;

	DepthStencilStateDesc depthStencilDesc = CommonStates::DepthStateReadWriteReversed();
	depthStencilDesc.depthFunc = ComparisonFunc::Always;
	depthStencilDesc.stencilEnable = true;
	depthStencilDesc.backFace.stencilFunc = ComparisonFunc::Always;
	depthStencilDesc.backFace.stencilDepthFailOp = StencilOp::Replace;
	depthStencilDesc.backFace.stencilFailOp = StencilOp::Replace;
	depthStencilDesc.backFace.stencilPassOp = StencilOp::IncrSat;
	depthStencilDesc.frontFace = depthStencilDesc.backFace;

	GraphicsPipelineDesc contourPipelineDesc{
		.name				= "Contour Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= depthStencilDesc,
		.rasterizerState	= rasterizerDesc,
		.rtvFormats			= { m_contourDataBuffer->GetFormat() },
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

	// Outer boundary init
	ComputePipelineDesc outerBoundaryDesc{
		.name			= "Outer boundary PSO",
		.computeShader	= { .shaderFile = "OuterContourCS" },
		.rootSignature	= m_outerBoundaryRootSig
	};
	m_outerBoundaryPipeline = m_app->CreateComputePipeline(outerBoundaryDesc);

	// Edge crossing
	ComputePipelineDesc edgeCrossingDesc{
		.name			= "Edge crossing PSO",
		.computeShader	= { .shaderFile = "EdgeCrossingCS" },
		.rootSignature	= m_edgeCrossingRootSig
	};
	m_edgeCrossingPipeline = m_app->CreateComputePipeline(edgeCrossingDesc);

	// Fill
	ComputePipelineDesc fillDesc{
		.name			= "Fill PSO",
		.computeShader	= { .shaderFile = "FillCS" },
		.rootSignature	= m_fillRootSig
	};
	m_fillPipeline = m_app->CreateComputePipeline(fillDesc);

	// Median filter
	ComputePipelineDesc medianFilterDesc{
		.name			= "Median filter PSO",
		.computeShader	= { .shaderFile = "MedianFilterCS" },
		.rootSignature	= m_medianFilterRootSig
	};
	m_medianFilterPipeline = m_app->CreateComputePipeline(medianFilterDesc);

	// Debug normals
	GraphicsPipelineDesc debugNormalsPipelineDesc{
		.name				= "Debug Normals Graphics PSO",
		.blendState			= CommonStates::BlendDisable(),
		.depthStencilState	= CommonStates::DepthStateReadWriteReversed(),
		.rasterizerState	= rasterizerDesc,
		.rtvFormats			= { m_app->GetColorFormat() },
		.dsvFormat			= m_app->GetDepthFormat(),
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "NormalDebugVS" },
		.pixelShader		= { .shaderFile = "NormalDebugPS" },
		.geometryShader		= { .shaderFile = "NormalDebugGS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= layout.GetElements(),
		.rootSignature		= m_debugNormalsRootSig
	};

	m_debugNormalsPipeline = m_app->CreateGraphicsPipeline(debugNormalsPipelineDesc);
}


void EndCapGenerator::InitBuffers()
{
	uint32_t width = m_app->GetWindowWidth();
	uint32_t height = m_app->GetWindowHeight();

	// Contour buffers
	ColorBufferDesc contourDataDesc{
		.name	= "Contour data buffer",
		.width	= width,
		.height = height,
		.format = Format::RGBA16_Float,
		.clearColor = DirectX::Colors::Transparent
	};
	m_contourDataBuffer = m_app->CreateColorBuffer(contourDataDesc);

	DepthBufferDesc depthBufferDesc{
		.name					= "Contour depth buffer",
		.width					= width,
		.height					= height,
		.format					= m_app->GetDepthFormat(),
		.createShaderResources	= true
	};
	m_depthBuffer = m_app->CreateDepthBuffer(depthBufferDesc);

	// Bounds buffer
	GpuBufferDesc boundsBufferDesc{
		.name					= "Bounds buffer",
		.resourceType			= ResourceType::TypedBuffer,
		.memoryAccess			= MemoryAccess::GpuReadWrite,
		.format					= Format::R32_SInt,
		.elementCount			= 4,
		.elementSize			= sizeof(int32_t),
		.bAllowShaderResource	= true,
		.bAllowUnorderedAccess	= true
	};
	m_boundsBuffer = m_app->CreateGpuBuffer(boundsBufferDesc);

	// Border buffer
	GpuBufferDesc borderBufferDesc{
		.name					= "Left border buffer",
		.resourceType			= ResourceType::TypedBuffer,
		.memoryAccess			= MemoryAccess::GpuReadWrite,
		.format					= Format::R32_SInt,
		.elementCount			= height,
		.elementSize			= sizeof(int32_t),
		.bAllowShaderResource	= true,
		.bAllowUnorderedAccess	= true
	};
	m_leftBorderBuffer = m_app->CreateGpuBuffer(borderBufferDesc);

	borderBufferDesc.name = "Right border buffer";
	m_rightBorderBuffer = m_app->CreateGpuBuffer(borderBufferDesc);

	borderBufferDesc.name = "Top border buffer";
	borderBufferDesc.elementCount = width;
	m_topBorderBuffer = m_app->CreateGpuBuffer(borderBufferDesc);

	borderBufferDesc.name = "Bottom border buffer";
	m_bottomBorderBuffer = m_app->CreateGpuBuffer(borderBufferDesc);

	// Outer boundary buffer
	ColorBufferDesc outerBoundaryDesc{
		.name		= "Outer boundary buffer",
		.width		= width,
		.height		= height,
		.format		= Format::R8_UInt
	};
	m_outerBoundaryBuffer = m_app->CreateColorBuffer(outerBoundaryDesc);

	// Edge crossing buffer
	ColorBufferDesc edgeCrossingDesc{
		.name		= "Edge crossing buffer",
		.width		= width,
		.height		= height,
		.format		= Format::RGBA8_UInt
	};
	m_edgeCrossingBuffer = m_app->CreateColorBuffer(edgeCrossingDesc);

	ColorBufferDesc edgeIdDesc{
		.name		= "Edge Id buffer",
		.width		= width,
		.height		= height,
		.format		= Format::R8_UInt
	};
	m_edgeIdBuffer = m_app->CreateColorBuffer(edgeIdDesc);

	ColorBufferDesc edgeCrossingDesc2{
		.name	= "Edge crossing buffer 2",
		.width		= width,
		.height		= height,
		.format		= Format::R8_UInt
	};
	m_edgeCrossingBuffer2 = m_app->CreateColorBuffer(edgeCrossingDesc2);

	// End-cap mask buffer
	ColorBufferDesc endCapMaskDesc{
		.name = "End-cap mask buffer",
		.width = width,
		.height = height,
		.format = Format::R8_UInt
	};
	m_endCapMaskBuffer = m_app->CreateColorBuffer(endCapMaskDesc);

	// Fill debug buffer
	ColorBufferDesc fillDebugDesc{
		.name = "Fill debug buffer",
		.width = width,
		.height = height,
		.format = Format::R8_UInt
	};
	m_fillDebugTex = m_app->CreateColorBuffer(fillDebugDesc);

	// Median filter buffer
	ColorBufferDesc filteredBufferDesc{
		.name	= "Filtered buffer",
		.width	= width,
		.height	= height,
		.format = Format::R8_UInt
	};
	m_filteredBuffer = m_app->CreateColorBuffer(filteredBufferDesc);
}


void EndCapGenerator::InitDescriptorSets()
{
	// Bounds init
	m_boundsInitDescriptors = m_boundsInitRootSig->CreateDescriptorSet(0);
	m_boundsInitDescriptors->SetUAV(0, m_boundsBuffer);

	// Border init
	m_leftRightBorderInitDescriptors = m_borderInitRootSig->CreateDescriptorSet(0);
	m_leftRightBorderInitDescriptors->SetUAV(0, m_leftBorderBuffer);
	m_leftRightBorderInitDescriptors->SetUAV(1, m_rightBorderBuffer);

	m_topBottomBorderInitDescriptors = m_borderInitRootSig->CreateDescriptorSet(0);
	m_topBottomBorderInitDescriptors->SetUAV(0, m_topBorderBuffer);
	m_topBottomBorderInitDescriptors->SetUAV(1, m_bottomBorderBuffer);

	// Contour pass
	m_psContourDescriptors = m_contourRootSignature->CreateDescriptorSet(2);
	m_psContourDescriptors->SetUAV(0, m_boundsBuffer);
	m_psContourDescriptors->SetUAV(1, m_leftBorderBuffer);
	m_psContourDescriptors->SetUAV(2, m_rightBorderBuffer);
	m_psContourDescriptors->SetUAV(3, m_topBorderBuffer);
	m_psContourDescriptors->SetUAV(4, m_bottomBorderBuffer);

	// Outer boundary
	m_outerBoundaryDescriptors = m_outerBoundaryRootSig->CreateDescriptorSet(0);
	m_outerBoundaryDescriptors->SetUAV(0, m_outerBoundaryBuffer);
	m_outerBoundaryDescriptors->SetSRV(0, m_leftBorderBuffer);
	m_outerBoundaryDescriptors->SetSRV(1, m_rightBorderBuffer);
	m_outerBoundaryDescriptors->SetSRV(2, m_topBorderBuffer);
	m_outerBoundaryDescriptors->SetSRV(3, m_bottomBorderBuffer);

	// Edge crossing filter
	m_edgeCrossingDescriptors = m_edgeCrossingRootSig->CreateDescriptorSet(0);
	m_edgeCrossingDescriptors->SetSRV(0, m_contourDataBuffer);
	m_edgeCrossingDescriptors->SetUAV(0, m_edgeCrossingBuffer);
	m_edgeCrossingDescriptors->SetUAV(1, m_edgeIdBuffer);
	m_edgeCrossingDescriptors->SetUAV(2, m_edgeCrossingBuffer2);
	m_edgeCrossingDescriptors->SetCBV(0, m_edgeCrossingConstantBuffer);

	// Fill
	m_fillDescriptors = m_fillRootSig->CreateDescriptorSet(0);
	m_fillDescriptors->SetUAV(0, m_endCapMaskBuffer);
	m_fillDescriptors->SetUAV(1, m_fillDebugTex);
	m_fillDescriptors->SetSRV(0, m_edgeCrossingBuffer2);
	m_fillDescriptors->SetSRV(1, m_depthBuffer, false);
	m_fillDescriptors->SetSRV(2, m_leftBorderBuffer);
	m_fillDescriptors->SetSRV(3, m_rightBorderBuffer);
	m_fillDescriptors->SetSRV(4, m_topBorderBuffer);
	m_fillDescriptors->SetSRV(5, m_bottomBorderBuffer);

	// Median filter
	m_medianFilterDescriptors = m_medianFilterRootSig->CreateDescriptorSet(0);
	m_medianFilterDescriptors->SetSRV(0, m_endCapMaskBuffer);
	m_medianFilterDescriptors->SetUAV(0, m_filteredBuffer);
	m_medianFilterDescriptors->SetCBV(0, m_medianFilterConstantBuffer);
}


void EndCapGenerator::UpdateConstantBuffers(float planeY, float normalLength)
{
	Matrix4 modelMatrix{ kIdentity };

	// Contours
	m_gsContourConstants.modelViewProjectionMatrix = m_app->GetCamera()->GetViewProjectionMatrix() * modelMatrix;
	m_gsContourConstants.modelViewProjectionInvTransposeMatrix = Transpose(Invert(m_gsContourConstants.modelViewProjectionMatrix));
	m_gsContourConstants.modelViewMatrix = m_app->GetCamera()->GetViewMatrix() * modelMatrix;
	m_gsContourConstants.modelMatrix = modelMatrix;

	Vector3 planeNormal = Vector3(0.0f, 1.0f, 0.0f);
	Vector3 pointOnPlane = Vector3(0.0f, planeY, 0.0f);
	m_gsContourConstants.plane = Vector4(planeNormal, -Dot(pointOnPlane, planeNormal));

	m_gsContourConstantBuffer->Update(sizeof(GSContourConstants), &m_gsContourConstants);

	// Edge crossing
	m_edgeCrossingConstants.texDimensions[0] = (float)m_contourDataBuffer->GetWidth();
	m_edgeCrossingConstants.texDimensions[1] = (float)m_contourDataBuffer->GetHeight();
	m_edgeCrossingConstants.invTexDimensions[0] = 1.0f / m_medianFilterConstants.texDimensions[0];
	m_edgeCrossingConstants.invTexDimensions[1] = 1.0f / m_medianFilterConstants.texDimensions[1];
	m_edgeCrossingConstantBuffer->Update(sizeof(EdgeCrossingConstants), &m_edgeCrossingConstants);

	// Median filter
	m_medianFilterConstants.texDimensions[0] = (float)m_contourDataBuffer->GetWidth();
	m_medianFilterConstants.texDimensions[1] = (float)m_contourDataBuffer->GetHeight();
	m_medianFilterConstants.invTexDimensions[0] = 1.0f / m_medianFilterConstants.texDimensions[0];
	m_medianFilterConstants.invTexDimensions[1] = 1.0f / m_medianFilterConstants.texDimensions[1];
	m_medianFilterConstantBuffer->Update(sizeof(MedianFilterConstants), &m_medianFilterConstants);

	// Debug normals
	m_gsDebugNormalsConstants.modelViewProjectionMatrix = m_app->GetCamera()->GetViewProjectionMatrix() * modelMatrix;
	m_gsDebugNormalsConstants.modelViewMatrix = m_app->GetCamera()->GetViewMatrix() * modelMatrix;
	m_gsDebugNormalsConstants.modelMatrix = modelMatrix;
	m_gsDebugNormalsConstants.plane = m_gsContourConstants.plane;
	m_gsDebugNormalsConstants.normalLength = normalLength;

	m_gsDebugNormalsConstantBuffer->Update(sizeof(GSDebugNormalsConstants), &m_gsDebugNormalsConstants);
}