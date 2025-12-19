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

#include "Graphics\GpuBuffer.h"
#include "Graphics\PipelineState.h"
#include "Graphics\RootSignature.h"


// Forward declarations
namespace Luna
{
class Application;
class GraphicsContext;
class IDevice;
} // namespace Luna


class FrustumVisualizer
{
	friend class MeshletCullApp;

public:
	FrustumVisualizer(Luna::Application* application);

	void Render(Luna::GraphicsContext& context);
	void Update(Math::Matrix4 viewProjectionMatrix, Math::Vector4 (&planes)[6]);

protected:
	void CreateDeviceDependentResources(Luna::IDevice* device);
	void CreateWindowSizeDependentResources(Luna::IDevice* device);

protected:
	Luna::Application* m_application{ nullptr };

	Luna::RootSignaturePtr m_rootSignature;
	Luna::MeshletPipelinePtr m_pipeline;

	_declspec(align(256u))
	struct Constants
	{
		Math::Matrix4 viewProjectionMatrix{ Math::kIdentity };
		Math::Vector4 planes[6];
		Math::Vector4 lineColor;
	};

	Constants m_constants;
	Luna::GpuBufferPtr m_constantBuffer;

	Luna::DescriptorSetPtr m_descriptorSet;
};