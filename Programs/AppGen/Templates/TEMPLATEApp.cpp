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

#include "TEMPLATEApp.h"

using namespace Luna;


TEMPLATEApp::TEMPLATEApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{
}


void TEMPLATEApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
}


void TEMPLATEApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void TEMPLATEApp::Startup()
{
	// Application initialization, after device creation
}


void TEMPLATEApp::Shutdown()
{
	// Application cleanup on shutdown
}


bool TEMPLATEApp::Update(double deltaTime)
{
	// Application update tick

	return true;
}


void TEMPLATEApp::Render()
{
	// Application main render loop
}