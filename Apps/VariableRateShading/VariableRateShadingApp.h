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


class VariableRateShadingApp : public Luna::Application
{
public:
	VariableRateShadingApp(uint32_t width, uint32_t height);

	int ProcessCommandLine(int argc, char* argv[]) final;

	void Configure() final;
	void Startup() final;
	void Shutdown() final;

	void Update() final;
	void Render() final;

protected:
	void CreateDeviceDependentResources() final;
	void CreateWindowSizeDependentResources() final;

	void InitLights();
	void InitRootSignature();
	void InitPipeline();

	void LoadAssets();

	void UpdateConstantBuffers();

	// How much to scale each dimension in the world.
	inline float GetWorldScale() const
	{
		return 0.1f;
	}

protected:
	struct Vertex
	{
		float pos[3];
		float normal[3];
		float uv[2];
		float tangent[3];
	};

	static constexpr int m_numLights{ 1 };

	struct LightState
	{
		Math::Vector4 position{};
		Math::Vector4 direction{};
		Math::Vector4 color{};
		Math::Vector4 falloff{};
		Math::Matrix4 viewMatrix{ Math::kIdentity };
		Math::Matrix4 projectionMatrix{ Math::kIdentity };
	};

	struct SceneConstants
	{
		Math::Matrix4 modelMatrix{ Math::kIdentity };
		Math::Matrix4 viewMatrix{ Math::kIdentity };
		Math::Matrix4 projectionMatrix{ Math::kIdentity };
		Math::Vector4 ambientColor{};
		int sampleShadowMap{ 0 };
		int padding[3] = { 0, 0, 0 };
		LightState lights[m_numLights];
		Math::Vector4 viewport{};
		Math::Vector4 clipPlane{};
	};

	Luna::GpuBufferPtr m_vertexBuffer;
	Luna::GpuBufferPtr m_indexBuffer;

	Luna::RootSignaturePtr m_sceneRootSignature;

	Luna::GraphicsPipelinePtr m_sceneGraphicsPipeline;
	bool m_pipelineCreated{ false };

	// Constant data
	SceneConstants m_sceneConstants{};
	SceneConstants m_shadowConstants{};

	// Constant buffers
	Luna::GpuBufferPtr m_sceneConstantBuffer;
	Luna::GpuBufferPtr m_shadowConstantBuffer;

	// Lights
	LightState m_lightState[m_numLights];
	Luna::Camera m_lightCameras[m_numLights];

	Math::BoundingBox m_sceneBoundingBox{};

	// Textures for scene objects
	std::vector<Luna::TexturePtr> m_textures;

	// Camera controller
	Luna::CameraController m_controller{ m_camera, Math::Vector3(Math::kYUnitVector) };

	// Application state
	bool m_animate{ true };
};