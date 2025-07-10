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

#include "Graphics\GraphicsCommon.h"
#include "Graphics\PipelineState.h"
#include "Graphics\ResourceSet.h"
#include "Graphics\Texture.h"


// Forward declarations
struct GLFWwindow;


namespace Luna
{

// Forward declarations
class GraphicsContext;


class UIOverlay
{
public:
	void Startup(GLFWwindow* window, GraphicsApi api, uint32_t width, uint32_t height, Format format, Format depthFormat);
	void Shutdown();

	void Update();
	void Render(GraphicsContext& context);

	void SetWindowSize(uint32_t width, uint32_t height);

	float GetScale() const { return m_scale; }

	bool Header(const char* caption);
	bool CheckBox(const char* caption, bool* value);
	bool CheckBox(const char* caption, int32_t* value);
	bool InputFloat(const char* caption, float* value, float step);
	bool SliderFloat(const char* caption, float* value, float min, float max);
	bool SliderInt(const char* caption, int32_t* value, int32_t min, int32_t max);
	bool ComboBox(const char* caption, int32_t* itemindex, std::vector<std::string> items);
	bool Button(const char* caption);
	void Text(const char* formatstr, ...);

protected:
	void InitImGui();
	void InitRootSig();
	void InitPSO();
	void InitFontTex();
	void InitConstantBuffer();
	void InitResourceSet();

	void UpdateConstantBuffer();

protected:
	struct Vertex
	{
		float position[2];
		float uv[2];
		uint32_t color;
	};

	struct VSConstants
	{
		Math::Matrix4 projectionMatrix;
	};

	RootSignaturePtr m_rootSignature;
	GraphicsPipelineStatePtr m_graphicsPipeline;

	TexturePtr m_fontTex;
	SamplerPtr m_fontSampler;

	VSConstants m_vsConstants;
	GpuBufferPtr m_vsConstantBuffer;

	ResourceSet	m_resources;

	GLFWwindow* m_window{ nullptr };
	GraphicsApi m_api{ GraphicsApi::D3D12 };
	float m_scale{ 1.0f };
	uint32_t m_width{ 0 };
	uint32_t m_height{ 0 };
	Format m_format;
	Format m_depthFormat;
};

// UI log category
inline LogCategory LogUI{ "LogUI" };

} // namespace Luna