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
#include "MeshletModel.h"


// Forward declarations
namespace Luna
{
class Application;
class GraphicsContext;
class IDevice;
} // namespace Luna


class CullDataVisualizer
{
	friend class MeshletCullApp;

public:
	CullDataVisualizer(Luna::Application* application);

	void Render(Luna::GraphicsContext& context, const MeshletMesh& mesh, uint32_t offset, uint32_t count);
	void Update(Math::Matrix4 worldMatrix, Math::Matrix4 viewMatrix, Math::Matrix4 projectionMatrix, Math::Vector4 color);

protected:
	void CreateDeviceDependentResources(Luna::IDevice* device);
	void CreateWindowSizeDependentResources(Luna::IDevice* device);

protected:
	Luna::Application* m_application{ nullptr };

	Luna::RootSignaturePtr m_rootSignature;
	Luna::MeshletPipelinePtr m_boundingSpherePipeline;
	Luna::MeshletPipelinePtr m_normalConePipeline;

	_declspec(align(256u))
	struct Constants
	{
		Math::Matrix4 worldMatrix{ Math::kIdentity };
		Math::Matrix4 viewProjectionMatrix{ Math::kIdentity };
		Math::Vector4 color{ Math::kZero };
		Math::Vector4 viewUpVector{ Math::kYUnitVector };
		Math::Vector4 viewForwardVector{ Math::kZUnitVector };
		float scale{ 0.0 };
	};

	Constants m_constants{};
	Luna::GpuBufferPtr m_constantBuffer;

	Luna::DescriptorSetPtr m_descriptorSet;
};