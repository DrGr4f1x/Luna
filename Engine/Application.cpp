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

#include <glfw\glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32 1
#include <glfw\glfw3native.h>

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
	: m_width{ width }
	, m_height{ height }
	, m_appTitle{ appTitle }
{}


Application::~Application()
{}


void Application::Configure()
{ }


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

	Finalize();
}

bool Application::Initialize()
{
	// Create core engine systems
	m_fileSystem = make_unique<FileSystem>(m_appTitle);
	m_fileSystem->SetDefaultRootPath();

	m_logSystem = make_unique<LogSystem>();

	// This is the first place we can post a startup message
	LogInfo(LogApplication) << "App " << m_appTitle << " initializing" << endl;

	// Application setup before device creation
	Configure();

	if (!CreateAppWindow())
	{
		return false;
	}

	HWND hwnd = glfwGetWin32Window(m_pWindow);
	m_inputSystem = make_unique<InputSystem>(hwnd);

	CreateDevice();

	// TODO: Update window size here

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

	double curTime = glfwGetTime();
	double deltaTime = curTime - m_prevFrameTime;

	m_inputSystem->Update(deltaTime);

	// Close on Escape key
	if (m_inputSystem->IsFirstPressed(DigitalInput::kKey_escape))
	{
		return false;
	}

	bool res = Update(deltaTime);
	if (res)
	{
		Render();
	}

	++m_frameNumber;
	m_prevFrameTime = curTime;

	return res;
}


bool Application::CreateAppWindow()
{
	glfwSetErrorCallback(GlfwErrorCallback);

	m_pWindow = glfwCreateWindow(m_width, m_height, m_appTitle.c_str(), nullptr, nullptr);

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


void Application::CreateDevice()
{
	// TODO
}


int Run(Application* pApplication)
{
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