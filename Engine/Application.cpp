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

using namespace std;


namespace Luna
{

Application::Application(uint32_t width, uint32_t height, const string& appTitle)
	: m_width{ width }
	, m_height{ height }
	, m_appTitle{ appTitle }
{}


void Application::Configure()
{ 
}


void Application::Run(int argc, char* argv[])
{
}

void Application::Initialize()
{
	// Create core engine systems
	m_fileSystem = make_unique<FileSystem>(m_appTitle);
	m_fileSystem->SetDefaultRootPath();

	//m_logSystem = make_unique<LogSystem>();

	// Application setup before device creation
	Configure();

	CreateDevice();

	Startup();

	m_bIsRunning = true;
}


void Application::Finalize()
{
	Shutdown();
}


void Application::CreateDevice()
{
	// TODO
}

} // namespace Luna