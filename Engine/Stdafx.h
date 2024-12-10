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

#define FORCE_VALIDATION 0
#define ENABLE_VALIDATION (_DEBUG || FORCE_VALIDATION)

#define FORCE_DEBUG_MARKERS 0
#define ENABLE_DEBUG_MARKERS (_DEBUG || _PROFILE || FORCE_DEBUG_MARKERS)

// Windows headers
#include <windows.h>
#include <wrl.h>
#include <Shlwapi.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <ppl.h>
#include <ppltasks.h>
#include <comdef.h>


// Standard library headers
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <future>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <shared_mutex>
#include <span>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <variant>
#include <vector>

// Core headers
#include "Core\BitmaskEnum.h"
#include "Core\Containers.h"
#include "Core\DWParam.h"
#include "Core\Hash.h"
#include "Core\IObject.h"
#include "Core\NonCopyable.h"
#include "Core\NonMovable.h"
#include "Core\VectorMath.h"

// Engine info
#define LUNA_MAKE_VERSION(major, minor, patch) \
    (((major) << 22) | ((minor) << 12) | (patch))
#define LUNA_VERSION_MAJOR(version) ((uint32_t)(version) >> 22)
#define LUNA_VERSION_MINOR(version) (((uint32_t)(version) >> 12) & 0x3ff)
#define LUNA_VERSION_PATCH(version) ((uint32_t)(version) & 0xfff)

constexpr uint32_t s_engineMajorVersion = 1;
constexpr uint32_t s_engineMinorVersion = 0;
constexpr uint32_t s_enginePatchVersion = 0;

constexpr uint32_t s_engineVersion = LUNA_MAKE_VERSION(s_engineMajorVersion, s_engineMinorVersion, s_enginePatchVersion);
static const std::string s_engineVersionStr
{
	std::to_string(s_engineMajorVersion) + "." + std::to_string(s_engineMinorVersion) + "." + std::to_string(s_enginePatchVersion)
};

static const std::string s_engineName{ "Luna" };

namespace Luna
{

void HelloEngine();

}