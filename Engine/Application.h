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

#include "StepTimer.h"
#include "Graphics\GraphicsCommon.h"
#include "Graphics\Camera.h"
#include "Graphics\ColorBuffer.h"
#include "Graphics\DepthBuffer.h"
#include "Graphics\GpuBuffer.h"
#include "Graphics\Grid.h"
#include "Graphics\Model.h"
#include "Graphics\PipelineState.h"
#include "Graphics\QueryHeap.h"
#include "Graphics\RootSignature.h"
#include "Graphics\Sampler.h"
#include "Graphics\Texture.h"
#include "Graphics\UIOverlay.h"


// Forward declarations
struct GLFWwindow;


namespace Luna
{

// Forward declarations
class FileSystem;
class InputSystem;
class LogSystem;
enum class GraphicsApi;


struct ApplicationInfo
{
	std::string name{ "Unnamed" };
	uint32_t width{ 1920 };
	uint32_t height{ 1080 };
	GraphicsApi api{ GraphicsApi::D3D12 };
#if ENABLE_VALIDATION
	bool useValidation{ true };
#else
	bool useValidation{ false };
#endif // ENABLE_VALIDATION

#if ENABLE_DEBUG_MARKERS
	bool useDebugMarkers{ true };
#else
	bool useDebugMarkers{ false };
#endif

	ApplicationInfo& SetName(const std::string& value) { name = value; return *this; }
	constexpr ApplicationInfo& SetWidth(uint32_t value) noexcept { width = value; return *this; }
	constexpr ApplicationInfo& SetHeight(uint32_t value) noexcept { height = value; return *this; }
	constexpr ApplicationInfo& SetApi(GraphicsApi value) noexcept { api = value; return *this; }
	constexpr ApplicationInfo& SetUseValidation(bool value) noexcept { useValidation = value; return *this; }
	constexpr ApplicationInfo& SetUseDebugMarkers(bool value) noexcept { useDebugMarkers = value; return *this; }
};


class FrameProCloser
{
public:
	~FrameProCloser();
};

bool IsFrameProRunning();


class Application
{
public:
	Application(uint32_t width, uint32_t height, const std::string& appTitle);
	virtual ~Application();

	virtual int ProcessCommandLine(int argc, char* argv[]);

	virtual void Configure();
	virtual void Startup() {}
	virtual void Shutdown() {}

	virtual void Update() {}
	virtual void UpdateUI() {}
	virtual void Render();

	virtual void OnWindowIconify(int iconified);
	virtual void OnWindowFocus(int focused);
	virtual void OnWindowRefresh();
	virtual void OnWindowClose();
	virtual void OnWindowPosition(int xPos, int yPos);
	virtual void OnMousePosition(uint32_t x, uint32_t y);

	void Run();

	// TODO: Think about where the authoritative window size is stored
	uint32_t GetWindowWidth() const { return m_appInfo.width; }
	uint32_t GetWindowHeight() const { return m_appInfo.height; }
	float GetWindowAspectRatio() const { return (float)m_appInfo.height / (float)m_appInfo.width; }

	Format GetColorFormat() const;
	Format GetDepthFormat() const;

	ColorBufferPtr GetColorBuffer() const;
	DepthBufferPtr GetDepthBuffer() const { return m_depthBuffer; }

	const ApplicationInfo& GetInfo() const { return m_appInfo; }

	// Wrappers for graphics resource creation
	ColorBufferPtr CreateColorBuffer(const ColorBufferDesc& colorBufferDesc);
	DepthBufferPtr CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc);
	GpuBufferPtr CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc);
	RootSignaturePtr CreateRootSignature(const RootSignatureDesc& rootSignatureDesc);
	GraphicsPipelinePtr CreateGraphicsPipeline(const GraphicsPipelineDesc& pipelineDesc);
	ComputePipelinePtr CreateComputePipeline(const ComputePipelineDesc& pipelineDesc);
	QueryHeapPtr CreateQueryHeap(const QueryHeapDesc& queryHeapDesc);
	TexturePtr CreateTexture1D(const TextureDesc& textureDesc);
	TexturePtr CreateTexture2D(const TextureDesc& textureDesc);
	TexturePtr CreateTexture3D(const TextureDesc& textureDesc);
	SamplerPtr CreateSampler(const SamplerDesc& samplerDesc);

	// Wrappers for resource loading
	TexturePtr LoadTexture(const std::string& filename, Format format = Format::Unknown, bool forceSrgb = false, bool retainData = false);
	ModelPtr LoadModel(const std::string& filename, const VertexLayoutBase& layout, float scale = 1.0f, ModelLoad loadFlags = ModelLoad::StandardDefault, bool loadMaterials = false);

protected:
	// Filesystem default configuration
	void SetDefaultRootPath();
	void SetDefaultSearchPaths();

	virtual void CreateDeviceDependentResources();
	virtual void CreateWindowSizeDependentResources();

	void UpdateWindowSize();

	void PrepareUI();
	void RenderUI(GraphicsContext& context);
	void RenderGrid(GraphicsContext& context);

protected:
	FrameProCloser m_frameProCloser{};

	ApplicationInfo m_appInfo;
	std::string m_appNameWithApi;

	HWND m_hwnd{};
	HINSTANCE m_hinst{};

	bool m_isRunning{ false };
	bool m_isVisible{ true };
	bool m_isWindowFocused{ false };
	bool m_showUI{ true };
	bool m_showGrid{ false };

	// Frame timer
	StepTimer m_timer;

	uint32_t m_mouseX{ 0 };
	uint32_t m_mouseY{ 0 };
	bool m_mouseMoveHandled{ false };

	// Camera
	Camera m_camera;

	// Engine systems
	std::unique_ptr<FileSystem> m_fileSystem;
	std::unique_ptr<LogSystem> m_logSystem;
	std::unique_ptr<InputSystem> m_inputSystem;

	wil::com_ptr<IDeviceManager> m_deviceManager;

	std::unique_ptr<UIOverlay> m_uiOverlay;
	std::unique_ptr<Grid> m_grid;

	// Rendering resources
	DepthBufferPtr m_depthBuffer;

private:
	bool Initialize();
	void Finalize();
	bool Tick();
	bool CreateAppWindow();
	void CreateDeviceManager();

private:
	GLFWwindow* m_pWindow{ nullptr };
};


int Run(Application* pApplication);

inline LogCategory LogApplication{ "LogApplication" };
inline LogCategory LogEngine{ "LogEngine" };
inline LogCategory LogGlfw{ "GLFW" };

} // namespace Luna