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


int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
	TEMPLATEApp app{ 1920, 1280 };
	Luna::Run(&app);

	return 0;
}