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
#include "Graphics\Formats.h"


namespace Luna
{

// Functions
bool IsDeveloperModeEnabled();
bool IsRenderDocAvailable();

inline bool IsTextureResource(ResourceType resourceType)
{
	using enum ResourceType;

	return (resourceType >= Texture1D) && (resourceType <= Texture3D);
}


inline bool IsTextureArray(ResourceType resourceType)
{
	using enum ResourceType;

	return
		resourceType == Texture1D_Array ||
		resourceType == Texture2D_Array ||
		resourceType == Texture2DMS_Array ||
		resourceType == TextureCube ||
		resourceType == TextureCube_Array;
}


inline bool IsBufferResource(ResourceType resourceType)
{
	using enum ResourceType;

	return (resourceType > Texture3D);
}


inline uint32_t ComputeNumMips(uint64_t width, uint32_t height)
{
	uint32_t highBit{ 0 };
	_BitScanReverse64((unsigned long*)&highBit, width | height);
	return highBit + 1;
}


struct IDeviceNotify
{
	virtual void OnDeviceLost() = 0;
	virtual void OnDeviceRestored() = 0;

protected:
	~IDeviceNotify() = default;
};


struct AdapterInfo
{
	std::string name{};
	uint32_t vendorId{ 0 };
	uint32_t deviceId{ 0 };
	size_t dedicatedVideoMemory{ 0 };
	size_t dedicatedSystemMemory{ 0 };
	size_t sharedSystemMemory{ 0 };
	HardwareVendor vendor{ HardwareVendor::Unknown };
	AdapterType adapterType{ AdapterType::Other };
	uint32_t apiVersion{ 0 };
};


DeviceManagerHandle CreateDeviceManager(const DeviceManagerDesc& desc);

// Graphics related log categories
inline LogCategory LogGraphics{ "LogGraphics" };

} // namespace Luna