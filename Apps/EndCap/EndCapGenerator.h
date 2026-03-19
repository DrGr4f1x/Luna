//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#pragma once

#include "Application.h"
#include "Graphics\ColorBuffer.h"
#include "Graphics\CommandContext.h"
#include "Graphics\DepthBuffer.h"
#include "Graphics\Model.h"
#include "Graphics\PipelineState.h"
#include "Graphics\RootSignature.h"


class EndCapGenerator
{
public:
	EndCapGenerator(Luna::Application* app);

	void CreateDeviceDependentResources();
	void CreateWindowSizeDependentResources();

	void Update(bool debugNormals, float planeY, float normalLength);
	void Render(Luna::GraphicsContext& context, Luna::Model* model, bool multipleModels);

	Luna::ColorBufferPtr GetEndCapMaskTexture() { return m_filteredBuffer; }

private:
	void InitRootSignatures();
	void InitPipelines();
	void InitBuffers();
	void InitDescriptorSets();

	void UpdateConstantBuffers(float planeY, float normalLength);

private:
	struct Vertex
	{
		float position[3];
		float color[4];
		float normal[3];
	};

	Luna::Application* m_app{ nullptr };
	bool m_debugNormals{ false };

	// Bounds init
	Luna::RootSignaturePtr m_boundsInitRootSig;
	Luna::ComputePipelinePtr m_boundsInitPipeline;

	Luna::DescriptorSetPtr m_boundsInitDescriptors;

	Luna::GpuBufferPtr m_boundsBuffer;

	// Border init
	Luna::RootSignaturePtr m_borderInitRootSig;
	Luna::ComputePipelinePtr m_borderInitPipeline;

	Luna::DescriptorSetPtr m_leftRightBorderInitDescriptors;
	Luna::DescriptorSetPtr m_topBottomBorderInitDescriptors;

	Luna::GpuBufferPtr m_leftBorderBuffer;
	Luna::GpuBufferPtr m_rightBorderBuffer;
	Luna::GpuBufferPtr m_topBorderBuffer;
	Luna::GpuBufferPtr m_bottomBorderBuffer;

	// Contour RS and PSO
	Luna::RootSignaturePtr m_contourRootSignature;
	Luna::GraphicsPipelinePtr m_contourPipeline;
	bool m_pipelineCreated{ false };

	// Contour constants
	struct GSContourConstants
	{
		Math::Matrix4 modelViewProjectionMatrix{ Math::kIdentity };
		Math::Matrix4 modelViewProjectionInvTransposeMatrix{ Math::kIdentity };
		Math::Matrix4 modelViewMatrix{ Math::kIdentity };
		Math::Matrix4 modelMatrix{ Math::kIdentity };
		Math::Vector4 plane{ Math::kZero };
	};

	Luna::GpuBufferPtr m_gsContourConstantBuffer;
	GSContourConstants m_gsContourConstants;

	Luna::DescriptorSetPtr m_psContourDescriptors;

	// Color and depth buffers
	Luna::ColorBufferPtr m_contourDataBuffer;
	Luna::DepthBufferPtr m_depthBuffer;

	// Outer boundary
	Luna::RootSignaturePtr m_outerBoundaryRootSig;
	Luna::ComputePipelinePtr m_outerBoundaryPipeline;

	Luna::ColorBufferPtr m_outerBoundaryBuffer;

	Luna::DescriptorSetPtr m_outerBoundaryDescriptors;

	// Edge crossing
	Luna::RootSignaturePtr m_edgeCrossingRootSig;
	Luna::ComputePipelinePtr m_edgeCrossingPipeline;

	Luna::ColorBufferPtr m_edgeCrossingBuffer;
	Luna::ColorBufferPtr m_edgeCrossingBuffer2;
	Luna::ColorBufferPtr m_edgeIdBuffer;

	struct EdgeCrossingConstants
	{
		float texDimensions[2] = { 0.0f, 0.0f };
		float invTexDimensions[2] = { 1.0f, 1.0f };
	};

	EdgeCrossingConstants m_edgeCrossingConstants;
	Luna::GpuBufferPtr m_edgeCrossingConstantBuffer;

	Luna::DescriptorSetPtr m_edgeCrossingDescriptors;

	// Fill
	Luna::RootSignaturePtr m_fillRootSig;
	Luna::ComputePipelinePtr m_fillPipeline;

	Luna::ColorBufferPtr m_endCapMaskBuffer;
	Luna::ColorBufferPtr m_fillDebugTex;

	Luna::DescriptorSetPtr m_fillDescriptors;

	// Median filter
	Luna::RootSignaturePtr m_medianFilterRootSig;
	Luna::ComputePipelinePtr m_medianFilterPipeline;

	struct MedianFilterConstants
	{
		float texDimensions[2] = { 0.0f, 0.0f };
		float invTexDimensions[2] = { 1.0f, 1.0f };
	};

	MedianFilterConstants m_medianFilterConstants;
	Luna::GpuBufferPtr m_medianFilterConstantBuffer;

	Luna::DescriptorSetPtr m_medianFilterDescriptors;

	Luna::ColorBufferPtr m_filteredBuffer;

	// Debug normals 
	Luna::RootSignaturePtr m_debugNormalsRootSig;
	Luna::GraphicsPipelinePtr m_debugNormalsPipeline;

	// Debug normal constants
	struct GSDebugNormalsConstants
	{
		Math::Matrix4 modelViewProjectionMatrix{ Math::kIdentity };
		Math::Matrix4 modelViewMatrix{ Math::kIdentity };
		Math::Matrix4 modelMatrix{ Math::kIdentity };
		Math::Vector4 plane{ Math::kZero };
		float normalLength{ 1.0f };
	};

	Luna::GpuBufferPtr m_gsDebugNormalsConstantBuffer;
	GSDebugNormalsConstants m_gsDebugNormalsConstants;
};