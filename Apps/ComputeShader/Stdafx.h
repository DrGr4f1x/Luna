//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

// Windows headers
#include <windows.h>
#include <wrl.h>
#include <wil\com.h>
#include <comdef.h>

#define USE_XINPUT
#include <XInput.h>
#pragma comment(lib, "xinput9_1_0.lib")

#define USE_KEYBOARD_MOUSE
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

// Standard library headers
#include <array>
#include <chrono>
#include <cstdarg>
#include <filesystem>
#include <format>
#include <fstream>
#include <future>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <variant>

#include "LunaFramePro.h"

// Engine headers
#include "Core\BitmaskEnum.h"
#include "Core\Containers.h"
#include "Core\CoreEnums.h"
#include "Core\DWParam.h"
#include "Core\Hash.h"
#include "Core\NativeObjectPtr.h"
#include "Core\NonCopyable.h"
#include "Core\NonMovable.h"
#include "Core\Profiling.h"
#include "Core\RefCounted.h"
#include "Core\Utility.h"
#include "Core\VectorMath.h"
#include "LogSystem.h"

// App name
static const std::string s_appName{ "ComputeShader" };