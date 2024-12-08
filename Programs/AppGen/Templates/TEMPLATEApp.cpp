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

using namespace Kodiak;
using namespace std;


int TEMPLATEApp::ProcessCommandLine(int argc, char* argv[])
{
	// Process commandline
	int res = Application::ProcessCommandLine(argc, argv);

	// Customize commandline here

	return res;
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


bool TEMPLATEApp::Update()
{
	// Application update tick

	return true;
}


void TEMPLATEApp::Render()
{
	// Application main render loop
}