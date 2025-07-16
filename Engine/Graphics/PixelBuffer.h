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


class IPixelBuffer : public IGpuResource
{
public:
	uint64_t GetWidth() const noexcept { return m_width; }
	uint32_t GetHeight() const noexcept { return m_height; }
	uint32_t GetDepth() const noexcept { return m_arraySizeOrDepth; }
	uint32_t GetArraySize() const noexcept { return m_arraySizeOrDepth; }
	uint32_t GetNumMips() const noexcept { return m_numMips; }
	uint32_t GetNumSamples() const noexcept { return m_numSamples; }
	uint32_t GetPlaneCount() const noexcept { return m_planeCount; }
	Format GetFormat() const noexcept { return m_format; }
	TextureDimension GetDimension() const noexcept { return m_dimension; }
	uint32_t GetNumFaces() const noexcept { return HasAnyFlag(m_type, ResourceType::TextureCube_Type) ? 6 : 1; }

protected:
	uint64_t m_width{ 0 };
	uint32_t m_height{ 0 };
	uint32_t m_arraySizeOrDepth{ 1 };
	uint32_t m_numMips{ 1 };
	uint32_t m_numSamples{ 1 };
	uint32_t m_planeCount{ 1 };
	Format m_format{ Format::Unknown };
	TextureDimension m_dimension{ TextureDimension::Unknown };
};

using PixelBufferPtr = std::shared_ptr<IPixelBuffer>;

} // namespace Luna