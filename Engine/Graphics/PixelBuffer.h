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
#include "Graphics\GpuResource.h"


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


class PixelBuffer : public GpuResource
{
public:
	PixelBuffer() noexcept;

	uint64_t GetWidth() const noexcept { return m_width; }
	uint32_t GetHeight() const noexcept { return m_height; }
	uint32_t GetDepth() const noexcept;
	uint32_t GetArraySize() const noexcept;
	uint32_t GetNumMips() const noexcept { return m_numMips; }
	uint32_t GetNumSamples() const noexcept { return m_numSamples; }
	uint32_t GetPlaneCount() const noexcept { return m_planeCount; }
	Format GetFormat() const noexcept { return m_format; }
	TextureDimension GetDimension() const noexcept;

	void Reset() noexcept;

protected:
	uint64_t m_width{ 0 };
	uint32_t m_height{ 0 };
	uint32_t m_arraySizeOrDepth{ 0 };
	uint32_t m_numMips{ 1 };
	uint32_t m_numSamples{ 1 };
	uint32_t m_planeCount{ 1 };
	Format m_format;
};

} // namespace Luna