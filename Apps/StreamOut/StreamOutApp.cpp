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

#include "StreamOutApp.h"

#include "Graphics\CommandContext.h"
#include "Graphics\CommonStates.h"

using namespace Luna;
using namespace Math;
using namespace std;


StreamOutApp::StreamOutApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{
}


int StreamOutApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void StreamOutApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void StreamOutApp::Startup()
{
	// Application initialization, after device creation
}


void StreamOutApp::Shutdown()
{
	// Application cleanup on shutdown
}


void StreamOutApp::Update()
{
	// Application update tick
	// Set m_bIsRunning to false if your application wants to exit
}


void StreamOutApp::Render()
{
	// Application main render loop
	Application::Render();
}


void StreamOutApp::CreateDeviceDependentResources()
{
	// Create any resources that depend on the device, but not the window size
}


void StreamOutApp::CreateWindowSizeDependentResources()
{
	// Create any resources that depend on window size.  May be called when the window size changes.
}