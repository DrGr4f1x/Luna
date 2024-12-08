//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Adapted from Utility.h in Miniengine
// https://github.com/Microsoft/DirectX-Graphics-Samples
//

#include "Stdafx.h"

#include "Utility.h"

#include <iostream>


void Utility::ExitFatal(const std::string& message, const std::string& caption)
{
	MessageBoxA(NULL, message.c_str(), caption.c_str(), MB_OK | MB_ICONERROR);
	std::cerr << message << std::endl;
	exit(1);
}