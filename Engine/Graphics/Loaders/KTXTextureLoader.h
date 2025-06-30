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

#include "Graphics\GraphicsCommon.h"


namespace Luna
{

// Forward declarations
class IDevice;
class ITexture;

bool CreateKTXTextureFromMemory(IDevice* device, ITexture* texture, const std::string& textureName, const uint8_t* data, size_t dataSize, Format format, bool forceSrgb);

} // namespace Luna