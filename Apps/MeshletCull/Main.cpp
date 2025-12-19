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

#include "MeshletCullApp.h"


int main(int argc, char* argv[])
{
	MeshletCullApp app{ 1280, 720 };
	
	app.ProcessCommandLine(argc, argv);

	return Luna::Run(&app);
}