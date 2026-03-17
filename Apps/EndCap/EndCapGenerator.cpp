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
		context.TransitionResource(m_edgeCullDebugBuffer, ResourceState::RenderTarget);
		context.TransitionResource(m_outerBoundaryBuffer, ResourceState::RenderTarget);
		context.TransitionResource(m_edgeCrossingBuffer, ResourceState::RenderTarget);
		context.TransitionResource(m_edgeIdBuffer, ResourceState::RenderTarget);
		context.TransitionResource(m_edgeDownsample8xBuffer, ResourceState::RenderTarget);
		context.TransitionResource(m_edgeDownsample64xBuffer, ResourceState::RenderTarget);
		context.TransitionResource(m_edgeCrossingBuffer2, ResourceState::RenderTarget);
		context.TransitionResource(m_endCapMaskBuffer, ResourceState::RenderTarget);
		context.TransitionResource(m_fillDebugTex, ResourceState::RenderTarget);
		context.ClearColor(m_contourDataBuffer);
		context.ClearColor(m_edgeCullDebugBuffer);
		context.ClearColor(m_outerBoundaryBuffer);
		context.ClearColor(m_edgeCrossingBuffer);
		context.ClearColor(m_edgeIdBuffer);
		context.ClearColor(m_edgeDownsample8xBuffer);
		context.ClearColor(m_edgeDownsample64xBuffer);
		context.ClearColor(m_edgeCrossingBuffer2);
		context.ClearColor(m_endCapMaskBuffer);
		context.ClearColor(m_fillDebugTex);

		context.ClearDepthAndStencil(m_depthBuffer);

		context.BeginRendering(m_contourDataBuffer, m_depthBuffer);
		context.SetViewportAndScissor(0u, 0u, width, height);

		context.SetStencilRef(0x1);

		// Draw the contour
		context.SetRootSignature(m_contourRootSignature);
		context.SetGraphicsPipeline(m_contourPipeline);

		context.SetRootCBV(1, m_gsContourConstantBuffer);
		context.SetDescriptors(2, m_psContourDescriptors);

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

	// Downsample
	{
		ScopedDrawEvent event2(computeContext, "Downsample");

		computeContext.TransitionResource(m_edgeIdBuffer, ResourceState::NonPixelShaderResource);
		computeContext.TransitionResource(m_edgeDownsample8xBuffer, ResourceState::UnorderedAccess);

		computeContext.SetRootSignature(m_edgeDownsampleRootSig);
		computeContext.SetComputePipeline(m_edgeDownsamplePipeline);

		computeContext.SetDescriptors(0, m_edgeDownsample8xDescriptors);

		computeContext.SetConstants(1, width, height);

		computeContext.Dispatch2D(width, height);

		computeContext.TransitionResource(m_edgeDownsample8xBuffer, ResourceState::NonPixelShaderResource);
		computeContext.TransitionResource(m_edgeDownsample64xBuffer, ResourceState::UnorderedAccess);

		computeContext.SetDescriptors(0, m_edgeDownsample64xDescriptors);

		const uint32_t width8 = Math::DivideByMultiple(width, 8);
		const uint32_t height8 = Math::DivideByMultiple(height, 8);

		computeContext.SetConstants(1, width8, height8);

		computeContext.Dispatch2D(width8, height8);
	}

	// Edge cull
	{
		ScopedDrawEvent event2(computeContext, "Edge Cull");

		context.TransitionResource(m_contourDataBuffer, ResourceState::UnorderedAccess);
		context.TransitionResource(m_edgeCrossingBuffer, ResourceState::NonPixelShaderResource);
		context.TransitionResource(m_edgeCullDebugBuffer, ResourceState::UnorderedAccess);
		context.TransitionResource(m_edgeIdBuffer, ResourceState::NonPixelShaderResource);

		computeContext.SetRootSignature(m_edgeCullRootSig);
		computeContext.SetComputePipeline(m_edgeCullPipeline);

		computeContext.SetDescriptors(0, m_edgeCullDescriptors);

		// Left-to-right
		{
			ScopedDrawEvent event3(computeContext, "Left-to-Right");
			computeContext.SetConstants(1, (int)width, (int)height, 0);
			computeContext.Dispatch1D(height, 64);
			computeContext.InsertUAVBarrier(m_contourDataBuffer);
			computeContext.InsertUAVBarrier(m_edgeCullDebugBuffer);
		}

		// Right-to-left
		{
			ScopedDrawEvent event3(computeContext, "Right-to-Left");
			computeContext.SetConstants(1, (int)width, (int)height, 1);
			computeContext.Dispatch1D(height, 64);
			computeContext.InsertUAVBarrier(m_contourDataBuffer);
			computeContext.InsertUAVBarrier(m_edgeCullDebugBuffer);
		}

		// Top-to-bottom
		{
			ScopedDrawEvent event3(computeContext, "Top-to-Bottom");
			computeContext.SetConstants(1, (int)width, (int)height, 2);
			computeContext.Dispatch1D(width, 64);
			computeContext.InsertUAVBarrier(m_contourDataBuffer);
			computeContext.InsertUAVBarrier(m_edgeCullDebugBuffer);
		}

		// Bottom-to-top
		{
			ScopedDrawEvent event3(computeContext, "Bottom-to-Top");
			computeContext.SetConstants(1, (int)width, (int)height, 3);
			computeContext.Dispatch1D(width, 64);
		}
	}

	// Jump flood init
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

		computeContext.TransitionResource(m_contourDataBuffer, ResourceState::NonPixelShaderResource);
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
			Table({TextureUAV(0, 2), TextureSRV, TypedBufferSRV(1, 4)}, ShaderStage::Compute),
			RootConstants(0, 3, ShaderStage::Compute)
		}
	};
	m_fillRootSig = m_app->CreateRootSignature(fillDesc);

	// Downsample 
	RootSignatureDesc downsampleDesc{
		.name				= "Edge crossing RS",
		.rootParameters		= {
			Table({ TextureSRV, TextureUAV }, ShaderStage::Compute),
			RootConstants(0, 2, ShaderStage::Compute)
		}
	};
	m_edgeDownsampleRootSig = m_app->CreateRootSignature(downsampleDesc);

	// Edge cull
	RootSignatureDesc edgeCullDesc{
		.name				= "Edge cull RS",
		.rootParameters		= {
			Table({ TextureUAV(0, 2), TextureSRV(0, 3), TypedBufferSRV(3, 4)}, ShaderStage::Compute),
			RootConstants(0, 3, ShaderStage::Compute)
		}
	};
	m_edgeCullRootSig = m_app->CreateRootSignature(edgeCullDesc);

	// Jump flood init
	RootSignatureDesc jumpFloodInitDesc{
		.name				= "Jump flood init RS",
		.rootParameters		= {
			Table({TextureSRV, TextureUAV, TextureUAV}, ShaderStage::Compute)
		}
	};

	m_jumpFloodInitRootSig = m_app->CreateRootSignature(jumpFloodInitDesc);

	// Jump flood main algorithm
	RootSignatureDesc jumpFloodDesc{
		.name				= "Jump flood RS",
		.rootParameters		= {
			Table({TextureSRV, TextureSRV, TextureUAV, TextureUAV, ConstantBuffer}, ShaderStage::Compute),
			RootConstants(1, 2, ShaderStage::Compute)
		}
	};

	m_jumpFloodRootSig = m_app->CreateRootSignature(jumpFloodDesc);

	// Median filter
	RootSignatureDesc medianFilterDesc{
		.name				= "Median filter RS",
		.rootParameters		= {
			Table({TextureSRV, TextureSRV, TextureUAV, ConstantBuffer}, ShaderStage::Compute)
		},
		.staticSamplers = { StaticSampler(CommonStates::SamplerLinearClamp()) }
	};

	m_medianFilterRootSig = m_app->CreateRootSignature(medianFilterDesc);
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
	//rasterizerDesc.multisampleEnable = true;

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

	// Downsample
	ComputePipelineDesc downsampleDesc{
		.name			= "Edge downsample PSO",
		.computeShader	= { .shaderFile = "DownsampleContoursCS" },
		.rootSignature	= m_edgeDownsampleRootSig
	};
	m_edgeDownsamplePipeline = m_app->CreateComputePipeline(downsampleDesc);

	// Edge cull
	ComputePipelineDesc edgeCullDesc{
		.name			= "Edge cull PSO",
		.computeShader	= { .shaderFile = "EdgeCullCS" },
		.rootSignature	= m_edgeCullRootSig
	};
	m_edgeCullPipeline = m_app->CreateComputePipeline(edgeCullDesc);

	// Jump flood init
	ComputePipelineDesc jumpFloodInitDesc{
		.name			= "Jump flood init PSO",
		.computeShader	= { .shaderFile = "JumpFloodInitCS" },
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
	ColorBufferDesc contourDataDesc{
		.name	= "Contour data buffer",
		.width	= width,
		.height = height,
		.format = Format::RGBA16_Float,
		.clearColor = DirectX::Colors::Transparent
	};
	m_contourDataBuffer = m_app->CreateColorBuffer(contourDataDesc);

	DepthBufferDesc depthBufferDesc{
		.name	= "Contour depth buffer",
		.width	= width,
		.height = height,
		.format = m_app->GetDepthFormat()
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

	// Downsample
	ColorBufferDesc downsample8xDesc{
		.name		= "Downsample 8x buffer",
		.width		= Math::DivideByMultiple(width, 8),
		.height		= Math::DivideByMultiple(height, 8),
		.format		= Format::R8_UInt
	};
	m_edgeDownsample8xBuffer = m_app->CreateColorBuffer(downsample8xDesc);

	ColorBufferDesc downsample64xDesc{
		.name		= "Downsample 64x buffer",
		.width		= Math::DivideByMultiple(width, 64),
		.height		= Math::DivideByMultiple(height, 64),
		.format		= Format::R8_UInt
	};
	m_edgeDownsample64xBuffer = m_app->CreateColorBuffer(downsample64xDesc);

	// Edge cull debug buffer
	ColorBufferDesc edgeCullDebugDesc{
		.name		= "Edge cull debug buffer",
		.width		= width,
		.height		= height,
		.format		= Format::RGBA8_UInt
	};
	m_edgeCullDebugBuffer = m_app->CreateColorBuffer(edgeCullDebugDesc);

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
	const uint32_t width = (uint32_t)m_contourDataBuffer->GetWidth();
	const uint32_t height = m_contourDataBuffer->GetHeight();
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
	m_fillDescriptors->SetSRV(1, m_leftBorderBuffer);
	m_fillDescriptors->SetSRV(2, m_rightBorderBuffer);
	m_fillDescriptors->SetSRV(3, m_topBorderBuffer);
	m_fillDescriptors->SetSRV(4, m_bottomBorderBuffer);

	// Downsample
	m_edgeDownsample8xDescriptors = m_edgeDownsampleRootSig->CreateDescriptorSet(0);
	m_edgeDownsample8xDescriptors->SetSRV(0, m_edgeIdBuffer);
	m_edgeDownsample8xDescriptors->SetUAV(0, m_edgeDownsample8xBuffer);

	m_edgeDownsample64xDescriptors = m_edgeDownsampleRootSig->CreateDescriptorSet(0);
	m_edgeDownsample64xDescriptors->SetSRV(0, m_edgeDownsample8xBuffer);
	m_edgeDownsample64xDescriptors->SetUAV(0, m_edgeDownsample64xBuffer);

	// Edge cull
	m_edgeCullDescriptors = m_edgeCullRootSig->CreateDescriptorSet(0);
	m_edgeCullDescriptors->SetUAV(0, m_contourDataBuffer);
	m_edgeCullDescriptors->SetUAV(1, m_edgeCullDebugBuffer);
	m_edgeCullDescriptors->SetSRV(0, m_edgeCrossingBuffer);
	m_edgeCullDescriptors->SetSRV(1, m_edgeIdBuffer);
	m_edgeCullDescriptors->SetSRV(2, m_edgeCrossingBuffer2);
	m_edgeCullDescriptors->SetSRV(3, m_leftBorderBuffer);
	m_edgeCullDescriptors->SetSRV(4, m_rightBorderBuffer);
	m_edgeCullDescriptors->SetSRV(5, m_topBorderBuffer);
	m_edgeCullDescriptors->SetSRV(6, m_bottomBorderBuffer);

	// Jump flood init
	m_jumpFloodInitDescriptors = m_jumpFloodInitRootSig->CreateDescriptorSet(0);
	m_jumpFloodInitDescriptors->SetSRV(0, m_contourDataBuffer);
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
	m_medianFilterDescriptors[0]->SetSRV(1, m_contourDataBuffer);
	m_medianFilterDescriptors[0]->SetUAV(0, m_filteredClassBuffer);
	m_medianFilterDescriptors[0]->SetCBV(0, m_medianFilterConstantBuffer);

	m_medianFilterDescriptors[1] = m_medianFilterRootSig->CreateDescriptorSet(0);
	m_medianFilterDescriptors[1]->SetSRV(0, m_jumpFloodClassBuffers[1]);
	m_medianFilterDescriptors[1]->SetSRV(1, m_contourDataBuffer);
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

	// Edge crossing
	m_edgeCrossingConstants.texDimensions[0] = (float)m_contourDataBuffer->GetWidth();
	m_edgeCrossingConstants.texDimensions[1] = (float)m_contourDataBuffer->GetHeight();
	m_edgeCrossingConstants.invTexDimensions[0] = 1.0f / m_medianFilterConstants.texDimensions[0];
	m_edgeCrossingConstants.invTexDimensions[1] = 1.0f / m_medianFilterConstants.texDimensions[1];
	m_edgeCrossingConstantBuffer->Update(sizeof(EdgeCrossingConstants), &m_edgeCrossingConstants);

	// Jump flood
	m_jumpFloodConstants.texWidth = (int)m_contourDataBuffer->GetWidth();
	m_jumpFloodConstants.texHeight = (int)m_contourDataBuffer->GetHeight();
	m_jumpFloodConstantBuffer->Update(sizeof(JumpFloodConstants), &m_jumpFloodConstants);

	// Median filter
	m_medianFilterConstants.texDimensions[0] = (float)m_contourDataBuffer->GetWidth();
	m_medianFilterConstants.texDimensions[1] = (float)m_contourDataBuffer->GetHeight();
	m_medianFilterConstants.invTexDimensions[0] = 1.0f / m_medianFilterConstants.texDimensions[0];
	m_medianFilterConstants.invTexDimensions[1] = 1.0f / m_medianFilterConstants.texDimensions[1];
	m_medianFilterConstantBuffer->Update(sizeof(MedianFilterConstants), &m_medianFilterConstants);
}