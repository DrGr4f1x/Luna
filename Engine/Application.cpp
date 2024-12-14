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
#include "Window.h"

#include <glfw\glfw3.h>

using namespace std;


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
{ 
}


void Application::Run()
{
	if (!Initialize())
	{
		return;
	}

	while (m_bIsRunning && !glfwWindowShouldClose(m_pWindow))
	{
		Update();

		Render();

		glfwPollEvents();
	}

	Finalize();
}

bool Application::Initialize()
{
	// Create core engine systems
	m_fileSystem = make_unique<FileSystem>(m_appTitle);
	m_fileSystem->SetDefaultRootPath();

	//m_logSystem = make_unique<LogSystem>();

	// Application setup before device creation
	Configure();

	if (!CreateAppWindow())
	{
		return false;
	}

	CreateDevice();

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


bool Application::CreateAppWindow()
{
	m_pWindow = glfwCreateWindow(m_width, m_height, m_appTitle.c_str(), nullptr, nullptr);

	return m_pWindow != nullptr;
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