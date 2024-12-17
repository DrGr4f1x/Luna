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

#include "Graphics\GpuImage.h"

namespace Luna
{

// Forward declarations
enum class Format : uint8_t;
enum class TextureDimension : uint8_t;


class __declspec(uuid("0A727C5B-352B-4781-9D4A-A8A3BA730FF0")) IPixelBuffer : public IGpuImage
{
public:
	virtual ~IPixelBuffer() = default;

	virtual uint64_t GetWidth() const noexcept = 0;
	virtual uint32_t GetHeight() const noexcept = 0;
	virtual uint32_t GetDepth() const noexcept = 0;
	virtual uint32_t GetArraySize() const noexcept = 0;
	virtual uint32_t GetNumMips() const noexcept = 0;
	virtual uint32_t GetNumSamples() const noexcept = 0;
	virtual Format GetFormat() const noexcept = 0;
	virtual uint32_t GetPlaneCount() const noexcept = 0;
	virtual TextureDimension GetDimension() const noexcept = 0;
};

} // namespace Luna