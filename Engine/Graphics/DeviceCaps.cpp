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

#include "DeviceCaps.h"

#include "GraphicsCommon.h"

using namespace std;


namespace Luna
{

void DeviceCaps::LogCaps()
{
	// TODO: Log basic device info here (driver, vendor, device name, etc)

	LogInfo(LogGraphics) << "Device Caps" << endl;
	LogInfo(LogGraphics) << "  Viewport" << endl;
	LogInfo(LogGraphics) << "    maxNum:    " << viewport.maxNum << endl;
	LogInfo(LogGraphics) << "    boundsMin: " << viewport.boundsMin << endl;
	LogInfo(LogGraphics) << "    boundsMax: " << viewport.boundsMax << endl;
}

} // namespace Luna