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

#include "Application.h"

#include "FileSystem.h"
#include "InputSystem.h"
#include "Window.h"

#include "Graphics\ColorBuffer.h"
#include "Graphics\CommandContext.h"

#include <glfw\glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32 1
#include <glfw\glfw3native.h>

#include <CLI11\CLI11.hpp>

#pragma comment(lib, "runtimeobject.lib")

using namespace std;


namespace 
{

// GLFW event callbacks

void GlfwErrorCallback(int errorCode, const char* description)
{
	using namespace Luna;
	LogError(LogGlfw) << description << endl;
}


void GlfwWindowIconifyCallback(GLFWwindow* pWindow, int iconified)
{
	using namespace Luna;
	auto pApplication = reinterpret_cast<Application*>(glfwGetWindowUserPointer(pWindow));
	pApplication->OnWindowIconify(iconified);
}


void GlfwWindowFocusCallback(GLFWwindow* pWindow, int focused)
{
	using namespace Luna;
	auto pApplication = reinterpret_cast<Application*>(glfwGetWindowUserPointer(pWindow));
	pApplication->OnWindowFocus(focused);
}


void GlfwWindowRefreshCallback(GLFWwindow* pWindow)
{
	using namespace Luna;
	auto pApplication = reinterpret_cast<Application*>(glfwGetWindowUserPointer(pWindow));
	pApplication->OnWindowRefresh();
}


void GlfwWindowCloseCallback(GLFWwindow* pWindow)
{
	using namespace Luna;
	auto pApplication = reinterpret_cast<Application*>(glfwGetWindowUserPointer(pWindow));
	pApplication->OnWindowClose();
}

void GlfwWindowPositionCallback(GLFWwindow* pWindow, int xPos, int yPos)
{
	using namespace Luna;
	auto pApplication = reinterpret_cast<Application*>(glfwGetWindowUserPointer(pWindow));
	pApplication->OnWindowPosition(xPos, yPos);
}

} // anonymous namespace


namespace Luna
{


Application::Application(uint32_t width, uint32_t height, const string& appTitle)
{
	m_appInfo.SetName(appTitle);
	m_appInfo.SetWidth(width);
	m_appInfo.SetHeight(height);
}


Application::~Application()
{}


int Application::ProcessCommandLine(int argc, char* argv[])
{
	CLI::App app{ "Luna App", m_appInfo.name };
	
	// Graphic API selection
	bool bDX12{ false };
	bool bVulkan{ false };
	auto dxOpt = app.add_flag("--dx,--dx12,--d3d12", bDX12, "Select DirectX renderer");
	auto vkOpt = app.add_flag("--vk,--vulkan", bVulkan, "Select Vulkan renderer");
	dxOpt->excludes(vkOpt);
	vkOpt->excludes(dxOpt);

	// Width, height
	auto widthOpt = app.add_option("--resx,--width", m_appInfo.width, "Sets initial window width");
	auto heightOpt = app.add_option("--resy,--height", m_appInfo.height, "Sets initial window height");

	// Parse command line
	CLI11_PARSE(app, argc, argv);

	// Set application parameters from command line
	m_appInfo.api = bVulkan ? GraphicsApi::Vulkan : GraphicsApi::D3D12;
	m_appNameWithApi = format("[{}] {}", GraphicsApiToString(m_appInfo.api), m_appInfo.name);

	return 0;
}


void Application::Configure()
{ }


void Application::Render()
{
	auto& context = GraphicsContext::Begin("Frame");

	context.TransitionResource(GetColorBuffer(), ResourceState::RenderTarget);
	context.ClearColor(GetColorBuffer(), DirectX::Colors::CornflowerBlue);

	// Rendering code goes here

	context.TransitionResource(GetColorBuffer(), ResourceState::Present);
	context.Finish();
}


void Application::OnWindowIconify(int iconified)
{ }


void Application::OnWindowFocus(int focused)
{ }


void Application::OnWindowRefresh()
{ }


void Application::OnWindowClose()
{ }


void Application::OnWindowPosition(int xPos, int yPos)
{ }


void Application::Run()
{
	Microsoft::WRL::Wrappers::RoInitializeWrapper InitializeWinRT(RO_INIT_MULTITHREADED);
	assert_succeeded(InitializeWinRT);

	m_hinst = GetModuleHandle(nullptr);

	if (!Initialize())
	{
		return;
	}

	while (m_bIsRunning && !glfwWindowShouldClose(m_pWindow))
	{
		glfwPollEvents();

		// TODO: Update window size here

		m_bIsRunning = Tick();
	}

	m_deviceManager->WaitForGpu();

	Finalize();
}


IColorBuffer* Application::GetColorBuffer() const
{
	return m_deviceManager->GetColorBuffer();
}


bool Application::Initialize()
{
	// Create core engine systems
	m_fileSystem = make_unique<FileSystem>(m_appInfo.name);
	m_fileSystem->SetDefaultRootPath();
	m_logSystem = make_unique<LogSystem>();

	// This is the first place we can post a startup message
	LogInfo(LogApplication) << "App " << m_appInfo.name << " initializing" << endl;

	// Application setup before device creation
	Configure();

	if (!CreateAppWindow())
	{
		return false;
	}

	m_hwnd = glfwGetWin32Window(m_pWindow);
	m_inputSystem = make_unique<InputSystem>(m_hwnd);

	CreateDeviceManager();
	CreateDeviceDependentResources();

	// TODO: Update window size here
	// UpdateWindowSize();
	m_deviceManager->CreateWindowSizeDependentResources();
	CreateWindowSizeDependentResources();

	Startup();

	m_bIsRunning = true;

	return true;
}


void Application::Finalize()
{
	Shutdown();

	glfwDestroyWindow(m_pWindow);
	m_pWindow = nullptr;
}


bool Application::Tick()
{
	if (!m_bIsRunning)
	{
		return false;
	}	

	// Close on Escape key
	if (m_inputSystem->IsFirstPressed(DigitalInput::kKey_escape))
	{
		return false;
	}

	// Tick the timer and update
	uint32_t frameCount = m_timer.GetFrameCount();
	m_timer.Tick([&]() { Update(); });

	// Render Frame
	{
		//m_deviceManager->BeginFrame();
		Render();
		m_deviceManager->Present();
	}

	if ((frameCount % 1000) == 0)
	{
		string windowTitle = format("{} - {} fps", m_appNameWithApi, m_timer.GetFramesPerSecond());
		glfwSetWindowTitle(m_pWindow, windowTitle.c_str());
	}

	return m_bIsRunning;
}


bool Application::CreateAppWindow()
{
	m_pWindow = glfwCreateWindow(m_appInfo.width, m_appInfo.height, m_appInfo.name.c_str(), nullptr, nullptr);

	if (m_pWindow == nullptr)
	{
		return false;
	}

	glfwSetWindowUserPointer(m_pWindow, this);
	glfwSetWindowIconifyCallback(m_pWindow, GlfwWindowIconifyCallback);
	glfwSetWindowFocusCallback(m_pWindow, GlfwWindowFocusCallback);
	glfwSetWindowRefreshCallback(m_pWindow, GlfwWindowRefreshCallback);
	glfwSetWindowCloseCallback(m_pWindow, GlfwWindowCloseCallback);
	glfwSetWindowPosCallback(m_pWindow, GlfwWindowPositionCallback);

	return true;
}


void Application::CreateDeviceManager()
{
	auto desc = DeviceManagerDesc{}
		.SetAppName(m_appInfo.name)
		.SetGraphicsApi(m_appInfo.api)
		.SetEnableValidation(m_appInfo.useValidation)
		.SetEnableDebugMarkers(m_appInfo.useDebugMarkers)
		.SetBackBufferWidth(m_appInfo.width)
		.SetBackBufferHeight(m_appInfo.height)
		.SetHwnd(m_hwnd)
		.SetHinstance(m_hinst);

	m_deviceManager = Luna::CreateDeviceManager(desc);
	m_deviceManager->CreateDeviceResources();
}


int Run(Application* pApplication)
{
	glfwSetErrorCallback(GlfwErrorCallback);

	if (!glfwInit())
	{
		return -1;
	}

	Window window{ pApplication };

	pApplication->Run();

	glfwTerminate();

	return 0;
}

} // namespace Luna