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

#include "Graphics\DeviceManager.h"
#include "Graphics\Enums.h"


namespace Luna
{

// Globals
inline const uint32_t g_numSwapChainBuffers{ 3 };


// Functions
bool IsDeveloperModeEnabled();
bool IsRenderDocAvailable();

} // namespace Luna