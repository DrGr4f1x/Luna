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

#include "TextureApp.h"

using namespace Luna;


TextureApp::TextureApp(uint32_t width, uint32_t height)
	: Application{ width, height, s_appName }
{
}


int TextureApp::ProcessCommandLine(int argc, char* argv[])
{
	// Handle commandline arguments here
	return Application::ProcessCommandLine(argc, argv);
}


void TextureApp::Configure()
{
	// Application config, before device creation
	Application::Configure();
}


void TextureApp::Startup()
{
	// Application initialization, after device creation
}


void TextureApp::Shutdown()
{
	// Application cleanup on shutdown
}


void TextureApp::Update()
{
	// Application update tick
	// Set m_bIsRunning to false if your application wants to exit
}


void TextureApp::Render()
{
	// Application main render loop
	Application::Render();
}


void TextureApp::CreateDeviceDependentResources()
{
	// Create any resources that depend on the device, but not the window size
}


void TextureApp::CreateWindowSizeDependentResources()
{
	// Create any resources that depend on window size.  May be called when the window size changes.
}