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

#include "Core\Utility.h"
#include "AppWindow.h"

#include <winrt/Windows.UI.Core.h>

using namespace winrt::Windows::UI::Core;


namespace Luna
{

Application::Application(uint32_t width, uint32_t height, const std::string& appTitle)
	: m_width{ width }
	, m_height{ height }
	, m_appTitle{ appTitle }
{}


void Application::Configure()
{ 
}


void Application::Run()
{
	while (m_bIsRunning && !m_bWindowClosed)
	{
		if (m_bIsVisible)
		{
			CoreWindow::GetForCurrentThread().Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

			bool res = Update();
			if (res)
			{
				// TODO
				// begin frame

				Render();

				// TODO
				// present
			}
			m_bIsRunning = res;
		}
		else
		{
			CoreWindow::GetForCurrentThread().Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
		}
		
	}

	Finalize();
}

void Application::Initialize()
{
	// TODO
	m_filesystem = make_unique<FileSystem>(m_appTitle);
	m_filesystem->SetDefaultRootPath();

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


void Run(Application* pApplication)
{
	using namespace winrt::Windows::ApplicationModel::Core;

	winrt::init_apartment();

	CoreApplication::Run(winrt::make<AppWindow>(pApplication));
}

} // namespace Luna