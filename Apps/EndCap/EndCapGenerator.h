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
	void Render(Luna::GraphicsContext& context, Luna::Model* model);

private:
	void InitRootSignatures();
	void InitPipelines();

	void UpdateConstantBuffers(float planeY);

private:
	struct Vertex
	{
		float position[3];
		float color[4];
		float normal[3];
	};

	Luna::Application* m_app{ nullptr };

	Luna::RootSignaturePtr m_contourRootSignature;
	Luna::GraphicsPipelinePtr m_contourPipeline;
	bool m_pipelineCreated{ false };

	// Contour constants
	struct ContourConstants
	{
		Math::Matrix4 viewProjectionMatrix{ Math::kIdentity };
		Math::Matrix4 modelMatrix{ Math::kIdentity };
		Math::Vector4 plane{ Math::kZero };
	};

	Luna::GpuBufferPtr m_contourConstantBuffer;
	ContourConstants m_contourConstants;
};