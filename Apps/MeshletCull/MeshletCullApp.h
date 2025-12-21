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
#include "CameraController.h"
#include "CullDataVisualizer.h"
#include "FrustumVisualizer.h"
#include "MeshletModel.h"
#include "Shaders/Shared.h"


class MeshletCullApp : public Luna::Application
{
public:
	MeshletCullApp(uint32_t width, uint32_t height);

	int ProcessCommandLine(int argc, char* argv[]) final;

	void Configure() final;
	void Startup() final;
	void Shutdown() final;

	void Update() final;
	void Render() final;

protected:
	void CreateDeviceDependentResources() final;
	void CreateWindowSizeDependentResources() final;

	void InitRootSignature();
	void InitPipeline();

	void LoadModels();

	void UpdateConstantBuffer();

	void Pick();
	Math::Vector4 GetSamplePoint();

protected:
	Luna::RootSignaturePtr m_rootSignature;
	Luna::MeshletPipelinePtr m_meshletPipeline;
	bool m_pipelineCreated{ false };

	struct SceneObject
	{
		MeshletModel model;
		Math::Matrix4 worldMatrix;
		Luna::GpuBufferPtr instanceBuffer;
		void* instanceData;
		uint32_t flags;
	};
	std::vector<SceneObject> m_objects;

	FrustumVisualizer m_frustumVisualizer{ this };
	CullDataVisualizer m_cullDataVisualizer{ this };

	Constants m_constants{};
	Luna::GpuBufferPtr m_constantBuffer;

	Luna::CameraController m_controller{ m_camera, Math::Vector3(Math::kYUnitVector) };

	Luna::Camera m_debugCamera;
	Luna::CameraController m_debugController{ m_debugCamera, Math::Vector3(Math::kYUnitVector) };

	// App state
	uint32_t m_highlightedIndex{ uint32_t(-1) };
	uint32_t m_selectedIndex{ uint32_t(-1) };
	bool m_drawMeshlets{ true };
};