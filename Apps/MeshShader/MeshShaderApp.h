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


class MeshShaderApp : public Luna::Application
{
public:
	MeshShaderApp(uint32_t width, uint32_t height);

	int ProcessCommandLine(int argc, char* argv[]) final;

	void Configure() final;
	void Startup() final;
	void Shutdown() final;

	void Update() final;
	void Render() final;

protected:
	void CreateDeviceDependentResources() final;
	void CreateWindowSizeDependentResources() final;

protected:
	struct Constants
	{
		Math::Matrix4 projectionMatrix{ Math::kIdentity };
		Math::Matrix4 modelMatrix{ Math::kIdentity };
		Math::Matrix4 viewMatrix{ Math::kIdentity };
	};
	Constants m_constants{};

	Luna::GpuBufferPtr m_constantBuffer;

	Luna::RootSignaturePtr m_rootSignature;

	Luna::CameraController m_controller{ m_camera, Math::Vector3(Math::kYUnitVector) };
};