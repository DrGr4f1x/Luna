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


// Forward declarations
struct GLFWwindow;


namespace Luna
{

// Forward declarations
class FileSystem;
class IColorBuffer;
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

	void Run();

	IColorBuffer* GetColorBuffer() const;

	const ApplicationInfo& GetInfo() const { return m_appInfo; }

protected:
	// Filesystem default configuration
	void SetDefaultRootPath();
	void SetDefaultSearchPaths();

	virtual void CreateDeviceDependentResources() {}
	virtual void CreateWindowSizeDependentResources() {}

	void UpdateWindowSize();

	Format GetColorFormat();
	Format GetDepthFormat();

protected:
	ApplicationInfo m_appInfo;
	std::string m_appNameWithApi;

	HWND m_hwnd{};
	HINSTANCE m_hinst{};

	bool m_bIsRunning{ false };
	bool m_bIsVisible{ true };
	bool m_bIsWindowFocused{ false };

	// Frame timer
	StepTimer m_timer;

	// Engine systems
	std::unique_ptr<FileSystem> m_fileSystem;
	std::unique_ptr<LogSystem> m_logSystem;
	std::unique_ptr<InputSystem> m_inputSystem;

	wil::com_ptr<IDeviceManager> m_deviceManager;

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