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

#include "..\Common\UWP\ViewProvider.h"


[Platform::MTAThread]
int WINAPIV main(Platform::Array<Platform::String^>^ /*params*/)
{
	TestApp app{ 1280, 720 };
	auto viewProvider = ref new Luna::ViewProvider(reinterpret_cast<UINT_PTR>(&app));

	Windows::ApplicationModel::Core::CoreApplication::Run(viewProvider);
	return 0;
}