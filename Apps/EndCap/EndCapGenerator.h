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

	void Update(float planeY);
	void Render(Luna::GraphicsContext& context, Luna::Model* model, bool multipleModels);

private:
	void InitRootSignatures();
	void InitPipelines();
	void InitBuffers();
	void InitJfaSteps();
	void InitDescriptorSets();

	void UpdateConstantBuffers(float planeY);

private:
	struct Vertex
	{
		float position[3];
		float color[4];
		float normal[3];
	};

	Luna::Application* m_app{ nullptr };

	// Contour RS and PSO
	Luna::RootSignaturePtr m_contourRootSignature;
	Luna::GraphicsPipelinePtr m_contourPipeline;
	bool m_pipelineCreated{ false };

	// Contour constants
	struct GSContourConstants
	{
		Math::Matrix4 modelViewProjectionMatrix{ Math::kIdentity };
		Math::Matrix4 modelViewMatrix{ Math::kIdentity };
		Math::Matrix4 modelMatrix{ Math::kIdentity };
		Math::Vector4 plane{ Math::kZero };
	};

	Luna::GpuBufferPtr m_gsContourConstantBuffer;
	GSContourConstants m_gsContourConstants;

	struct PSContourConstants
	{
		Math::Matrix4 modelViewMatrix{ Math::kIdentity };
		Math::Vector4 worldUpVector{ Math::kYUnitVector };
		Math::Vector4 viewPos{ Math::kZero };
	};

	Luna::GpuBufferPtr m_psContourConstantBuffer;
	PSContourConstants m_psContourConstants;

	// Color and depth buffers
	Luna::ColorBufferPtr m_colorBuffer;
	Luna::ColorBufferPtr m_normalBuffer;
	Luna::DepthBufferPtr m_depthBuffer;

	// Jump flood RS and PSOs
	Luna::RootSignaturePtr m_jumpFloodInitRootSig;
	Luna::ComputePipelinePtr m_jumpFloodInitPipeline;

	Luna::RootSignaturePtr m_jumpFloodRootSig;
	Luna::ComputePipelinePtr m_jumpFloodPipeline;

	// Jump flood buffers
	Luna::ColorBufferPtr m_jumpFloodDataBuffers[2];
	Luna::ColorBufferPtr m_jumpFloodClassBuffers[2];

	// Jump flood init descriptor set
	Luna::DescriptorSetPtr m_jumpFloodInitDescriptors;

	struct JumpFloodConstants
	{
		int texWidth{ 1 };
		int texHeight{ 1 };
	};

	JumpFloodConstants m_jumpFloodConstants;
	Luna::GpuBufferPtr m_jumpFloodConstantBuffer;

	std::vector<std::pair<uint32_t, uint32_t>> m_jfaSteps;

	std::vector<Luna::DescriptorSetPtr> m_jumpFloodDescriptors;

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

	Luna::DescriptorSetPtr m_medianFilterDescriptors[2];

	Luna::ColorBufferPtr m_filteredClassBuffer;
};