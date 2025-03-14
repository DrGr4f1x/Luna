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

#include "Graphics\GpuResource.h"


namespace Luna
{

// Forward declarations
enum class Format : uint8_t;
enum class TextureDimension : uint8_t;


class __declspec(uuid("1E794589-C1F0-40DE-9CFE-45696525E863")) IPixelBuffer : public IGpuResource
{
public:
	virtual uint64_t GetWidth() const noexcept = 0;
	virtual uint32_t GetHeight() const noexcept = 0;
	virtual uint32_t GetDepth() const noexcept = 0;
	virtual uint32_t GetArraySize() const noexcept = 0;
	virtual uint32_t GetNumMips() const noexcept = 0;
	virtual uint32_t GetNumSamples() const noexcept = 0;
	virtual uint32_t GetPlaneCount() const noexcept = 0;
	virtual Format GetFormat() const noexcept = 0;
	virtual TextureDimension GetDimension() const noexcept = 0;
};

using PixelBufferHandle = wil::com_ptr<IPixelBuffer>;

} // namespace Luna