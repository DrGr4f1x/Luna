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

#include "UIOverlay.h"

#include "Filesystem.h"
#include "CommandContext.h"
#include "CommonStates.h"
#include "Device.h"
#include "DeviceManager.h"
#include "GpuBuffer.h"
#include "Sampler.h"

#include "imgui.h"
#include "backends\imgui_impl_glfw.h"


namespace Luna
{

void UIOverlay::Startup(GLFWwindow* window, GraphicsApi api, uint32_t width, uint32_t height, Format format, Format depthFormat)
{
	LogNotice(LogUI) << "Starting up UI overlay" << std::endl;

	m_window = window;
	m_api = api;
	m_width = width;
	m_height = height;
	m_format = format;
	m_depthFormat = depthFormat;

	InitImGui();
	InitRootSig();
	InitPSO();
	InitFontTex();
	InitConstantBuffer();
	InitResourceSet();
}


void UIOverlay::Shutdown()
{
	LogNotice(LogUI) << "Shutting down UI overlay" << std::endl;
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}


void UIOverlay::Update()
{
	UpdateConstantBuffer();
}


void UIOverlay::Render(GraphicsContext& context)
{
	ImDrawData* imDrawData = ImGui::GetDrawData();

	if (!imDrawData || imDrawData->CmdListsCount == 0)
		return;

	ScopedDrawEvent event(context, "UI Overlay");

	context.SetViewportAndScissor(0u, 0u, m_width, m_height);

	context.SetRootSignature(m_rootSignature);
	context.SetGraphicsPipeline(m_graphicsPipeline);

	context.SetPrimitiveTopology(PrimitiveTopology::TriangleList);

	context.SetResources(m_resources);

	// Gather dynamic vertices and indices
	{
		DynAlloc vb = context.ReserveUploadMemory(imDrawData->TotalVtxCount * sizeof(ImDrawVert));
		DynAlloc ib = context.ReserveUploadMemory(imDrawData->TotalIdxCount * sizeof(ImDrawIdx));

		ImDrawVert* vtxDst = (ImDrawVert*)vb.dataPtr;
		ImDrawIdx* idxDst = (ImDrawIdx*)ib.dataPtr;

		for (int i = 0; i < imDrawData->CmdListsCount; ++i)
		{
			const ImDrawList* cmd_list = imDrawData->CmdLists[i];
			memcpy(vtxDst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
			memcpy(idxDst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
			vtxDst += cmd_list->VtxBuffer.Size;
			idxDst += cmd_list->IdxBuffer.Size;
		}

		context.SetDynamicVertexBuffer(0, imDrawData->TotalVtxCount, sizeof(ImDrawVert), vb);
		const bool bIndexSize16Bit = sizeof(ImDrawIdx) == sizeof(uint16_t);
		context.SetDynamicIndexBuffer(imDrawData->TotalIdxCount, bIndexSize16Bit, ib);
	}

	ImGuiIO& io = ImGui::GetIO();

	uint32_t indexOffset = 0;
	int32_t vertexOffset = 0;
	for (int i = 0; i < imDrawData->CmdListsCount; ++i)
	{
		const ImDrawList* cmd_list = imDrawData->CmdLists[i];
		for (int j = 0; j < cmd_list->CmdBuffer.Size; ++j)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[j];
			context.SetScissor(
				std::max<uint32_t>((uint32_t)pcmd->ClipRect.x, 0u),
				std::max<uint32_t>((uint32_t)pcmd->ClipRect.y, 0u),
				(uint32_t)pcmd->ClipRect.z,
				(uint32_t)pcmd->ClipRect.w);
			context.DrawIndexed((uint32_t)pcmd->ElemCount, indexOffset, vertexOffset);
			indexOffset += pcmd->ElemCount;
		}
		vertexOffset += cmd_list->VtxBuffer.Size;
	}
}


void UIOverlay::SetWindowSize(uint32_t width, uint32_t height)
{
	m_width = width;
	m_height = height;
}


bool UIOverlay::Header(const char* caption)
{
	return ImGui::CollapsingHeader(caption, ImGuiTreeNodeFlags_DefaultOpen);
}


bool UIOverlay::CheckBox(const char* caption, bool* value)
{
	return ImGui::Checkbox(caption, value);
}


bool UIOverlay::CheckBox(const char* caption, int32_t* value)
{
	bool val = (*value == 1);
	bool res = ImGui::Checkbox(caption, &val);
	*value = val;
	return res;
}


bool UIOverlay::InputFloat(const char* caption, float* value, float step)
{
	return ImGui::InputFloat(caption, value, step, step * 10.0f);
}


bool UIOverlay::SliderFloat(const char* caption, float* value, float min, float max)
{
	return ImGui::SliderFloat(caption, value, min, max);
}


bool UIOverlay::SliderInt(const char* caption, int32_t* value, int32_t min, int32_t max)
{
	return ImGui::SliderInt(caption, value, min, max);
}


bool UIOverlay::ComboBox(const char* caption, int32_t* itemindex, std::vector<std::string> items)
{
	if (items.empty())
		return false;

	std::vector<const char*> charitems;
	charitems.reserve(items.size());
	for (size_t i = 0; i < items.size(); ++i)
	{
		charitems.push_back(items[i].c_str());
	}
	uint32_t itemCount = static_cast<uint32_t>(charitems.size());

	return ImGui::Combo(caption, itemindex, &charitems[0], itemCount, itemCount);
}


bool UIOverlay::Button(const char* caption)
{
	return ImGui::Button(caption);
}


void UIOverlay::Text(const char* formatstr, ...)
{
	va_list args;
	va_start(args, formatstr);
	ImGui::TextV(formatstr, args);
	va_end(args);
}


static inline ImVec4 ColorToImVec4(const Color& color, float alpha)
{
	return ImVec4(color.R(), color.G(), color.B(), alpha);
}


void UIOverlay::InitImGui()
{
	// Init ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// Color scheme
	ImGuiStyle& style = ImGui::GetStyle();

	Color colorBright;
	Color colorDark;

	if (m_api == GraphicsApi::D3D12)
	{
		colorBright = Color(0.055f, 0.478f, 0.051f);
		colorDark = Color(0.044f, 0.382f, 0.0408f);
	}
	else
	{
		colorBright = Color(1.0f, 0.0f, 0.0f);
		colorDark = Color(0.8f, 0.0f, 0.0f);
	}

	style.Colors[ImGuiCol_TitleBg] = ColorToImVec4(colorBright, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive] = ColorToImVec4(colorBright, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ColorToImVec4(colorBright, 0.1f);
	style.Colors[ImGuiCol_MenuBarBg] = ColorToImVec4(colorBright, 0.4f);
	style.Colors[ImGuiCol_Header] = ColorToImVec4(colorDark, 0.4f);
	style.Colors[ImGuiCol_HeaderActive] = ColorToImVec4(colorBright, 0.4f);
	style.Colors[ImGuiCol_HeaderHovered] = ColorToImVec4(colorBright, 0.4f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.8f);
	style.Colors[ImGuiCol_CheckMark] = ColorToImVec4(colorBright, 0.8f);
	style.Colors[ImGuiCol_SliderGrab] = ColorToImVec4(colorBright, 0.4f);
	style.Colors[ImGuiCol_SliderGrabActive] = ColorToImVec4(colorBright, 0.8f);
	style.Colors[ImGuiCol_FrameBgHovered] = ColorToImVec4(colorBright, 0.1f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
	style.Colors[ImGuiCol_Button] = ColorToImVec4(colorBright, 0.4f);
	style.Colors[ImGuiCol_ButtonHovered] = ColorToImVec4(colorBright, 0.6f);
	style.Colors[ImGuiCol_ButtonActive] = ColorToImVec4(colorBright, 0.8f);

	// Dimensions
	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = m_scale;

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	if (m_api == GraphicsApi::Vulkan)
	{
		ImGui_ImplGlfw_InitForVulkan(m_window, true);
	}
	else
	{
		ImGui_ImplGlfw_InitForOther(m_window, true);
	}
}


void UIOverlay::InitRootSig()
{
	auto rootSignatureDesc = RootSignatureDesc{
		.name = "Root Sig",
		.flags = RootSignatureFlags::AllowInputAssemblerInputLayout,
		.rootParameters =
			{
				RootParameter::RootCBV(0, ShaderStage::Vertex),
				RootParameter::Table({ DescriptorRange::TextureSRV(0) }, ShaderStage::Pixel),
				RootParameter::Table({ DescriptorRange::Sampler(0) }, ShaderStage::Pixel)
			}
	};

	auto device = GetDeviceManager()->GetDevice();

	m_rootSignature = device->CreateRootSignature(rootSignatureDesc);
}


void UIOverlay::InitPSO()
{
	VertexStreamDesc vertexStreamDesc{
		.inputSlot				= 0,
		.stride					= sizeof(Vertex),
		.inputClassification	= InputClassification::PerVertexData
	};

	std::vector<VertexElementDesc> vertexElements{
		{ "POSITION", 0, Format::RGB32_Float, 0, offsetof(Vertex, position), InputClassification::PerVertexData, 0 },
		{ "TEXCOORD", 0, Format::RG32_Float, 0, offsetof(Vertex, uv), InputClassification::PerVertexData, 0 },
		{ "COLOR", 0, Format::RGBA8_UNorm, 0, offsetof(Vertex, color), InputClassification::PerVertexData, 0 }
	};

	// Custom blend state
	BlendStateDesc blendStateDesc{};
	blendStateDesc.alphaToCoverageEnable = false;
	blendStateDesc.independentBlendEnable = false;
	for (auto& renderTargetBlend : blendStateDesc.renderTargetBlend)
	{
		renderTargetBlend.blendEnable = true;
		renderTargetBlend.srcBlend = Blend::SrcAlpha;
		renderTargetBlend.dstBlend = Blend::InvSrcAlpha;
		renderTargetBlend.blendOp = BlendOp::Add;
		renderTargetBlend.srcBlendAlpha = Blend::InvSrcAlpha;
		renderTargetBlend.dstBlendAlpha = Blend::Zero;
		renderTargetBlend.blendOpAlpha = BlendOp::Add;
		renderTargetBlend.writeMask = ColorWrite::All;
	}

	GraphicsPipelineDesc desc
	{
		.name				= "UIOverlay PSO",
		.blendState			= blendStateDesc,
		.depthStencilState	= CommonStates::DepthStateReadOnlyReversed(),
		.rasterizerState	= CommonStates::RasterizerTwoSided(),
		.rtvFormats			= { m_format },
		.dsvFormat			= m_depthFormat,
		.topology			= PrimitiveTopology::TriangleList,
		.vertexShader		= { .shaderFile = "UIVS" },
		.pixelShader		= { .shaderFile = "UIPS" },
		.vertexStreams		= { vertexStreamDesc },
		.vertexElements		= vertexElements,
		.rootSignature		= m_rootSignature
	};

	auto device = GetDeviceManager()->GetDevice();
	m_graphicsPipeline = device->CreateGraphicsPipelineState(desc);
}


void UIOverlay::InitFontTex()
{
	ImGuiIO& io = ImGui::GetIO();

	auto filesystem = GetFileSystem();
	auto device = GetDeviceManager()->GetDevice();

	std::string fullPath = filesystem->GetFullPath("Roboto-Medium.ttf");
	io.Fonts->AddFontFromFileTTF(fullPath.c_str(), 16.0f);

	unsigned char* fontData{ nullptr };
	int texWidth{ 0 };
	int texHeight{ 0 };

	io.Fonts->GetTexDataAsRGBA32(&fontData, &texWidth, &texHeight);

	TextureDesc fontDesc{
		.name		= "Roboto-Medium.ttf",
		.width		= (uint64_t)texWidth,
		.height		= (uint32_t)texHeight,
		.depth		= 1,
		.format		= Format::RGBA8_UNorm,
		.data		= (std::byte*)fontData
	};

	m_fontTex = device->CreateTexture2D(fontDesc);

	SamplerDesc samplerDesc = CommonStates::SamplerLinearBorder();
	m_fontSampler = device->CreateSampler(samplerDesc);
}


void UIOverlay::InitConstantBuffer()
{
	GpuBufferDesc constantBufferDesc{
		.name			= "UIOverlay Constant Buffer",
		.resourceType	= ResourceType::ConstantBuffer,
		.memoryAccess	= MemoryAccess::GpuRead | MemoryAccess::CpuWrite,
		.elementCount	= 1,
		.elementSize	= sizeof(VSConstants)
	};

	auto device = GetDeviceManager()->GetDevice();
	m_vsConstantBuffer = device->CreateGpuBuffer(constantBufferDesc);
}


void UIOverlay::InitResourceSet()
{
	m_resources.Initialize(m_rootSignature);
	m_resources.SetCBV(0, 0, m_vsConstantBuffer);
	m_resources.SetSRV(1, 0, m_fontTex);
	m_resources.SetSampler(2, 0, m_fontSampler);
}


void UIOverlay::UpdateConstantBuffer()
{
	using namespace Math;

	const ImDrawData* imDrawData = ImGui::GetDrawData();

	const float L = imDrawData->DisplayPos.x;
	const float R = L + imDrawData->DisplaySize.x;
	const float T = imDrawData->DisplayPos.y;
	const float B = T + imDrawData->DisplaySize.y;

	m_vsConstants.projectionMatrix.SetX(Vector4(2.0f / (R - L), 0.0f, 0.0f, 0.0f));
	m_vsConstants.projectionMatrix.SetY(Vector4(0.0f, 2.0f / (T - B), 0.0f, 0.0f));
	m_vsConstants.projectionMatrix.SetZ(Vector4(0.0f, 0.0f, 0.5f, 0.0f));
	m_vsConstants.projectionMatrix.SetW(Vector4((R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f));

	m_vsConstantBuffer->Update(sizeof(VSConstants), &m_vsConstants);
}

} // namespace Luna