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

#include "AppWindow.h"

using namespace Luna;
using namespace winrt::Windows::ApplicationModel::Core;


int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
	winrt::init_apartment();

	TestApp app{ 1920, 1280 };
	CoreApplication::Run(winrt::make<AppWindow>(&app));

	return 0;
}