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

#include "TestApp.h"

using namespace Luna;


TestApp::TestApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{
}


int TestApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void TestApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void TestApp::Startup()
{
	// Application initialization, after device creation
}


void TestApp::Shutdown()
{
	// Application cleanup on shutdown
}


bool TestApp::Update(double deltaTime)
{
	// Application update tick

	return true;
}


void TestApp::Render()
{
	// Application main render loop
}


void TestApp::CreateDeviceDependentResources()
{
	// Create any resources that depend on the device, but not the window size
}


void TestApp::CreateWindowSizeDependentResources()
{
	// Create any resources that depend on window size.  May be called when the window size changes.
}