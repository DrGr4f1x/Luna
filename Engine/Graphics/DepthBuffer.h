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

#include "Graphics\Enums.h"
#include "Graphics\Formats.h"
#include "Graphics\PixelBuffer.h"

namespace Luna
{

struct DepthBufferDesc
{
	std::string name;
	ResourceType resourceType{ ResourceType::Texture2D };
	uint64_t width{ 0 };
	uint32_t height{ 0 };
	uint32_t arraySizeOrDepth{ 0 };
	uint32_t numMips{ 1 };
	uint32_t numSamples{ 1 };
	Format format{ Format::Unknown };
	float clearDepth{ 1.0f };
	uint8_t clearStencil{ 0 };

	DepthBufferDesc& SetName(const std::string& value) { name = value; return *this; }
	constexpr DepthBufferDesc& SetResourceType(ResourceType value) noexcept { resourceType = value; return *this; }
	constexpr DepthBufferDesc& SetWidth(uint64_t value) noexcept { width = value; return *this; }
	constexpr DepthBufferDesc& SetHeight(uint32_t value) noexcept { height = value; return *this; }
	constexpr DepthBufferDesc& SetArraySize(uint32_t value) noexcept { arraySizeOrDepth = value; return *this; }
	constexpr DepthBufferDesc& SetDepth(uint32_t value) noexcept { arraySizeOrDepth = value; return *this; }
	constexpr DepthBufferDesc& SetNumMips(uint32_t value) noexcept { numMips = value; return *this; }
	constexpr DepthBufferDesc& SetNumSamples(uint32_t value) noexcept { numSamples = value; return *this; }
	constexpr DepthBufferDesc& SetFormat(Format value) noexcept { format = value; return *this; }
	constexpr DepthBufferDesc& SetClearDepth(float value) noexcept { clearDepth = value; return *this; }
	constexpr DepthBufferDesc& SetClearStencil(uint8_t value) noexcept { clearStencil = value; return *this; }
};


class DepthBuffer : public PixelBuffer
{
public:
	float GetClearDepth() const noexcept { return m_clearDepth; }
	uint8_t GetClearStencil() const noexcept { return m_clearStencil; }

	bool Initialize(DepthBufferDesc& desc);
	bool IsInitialized() const noexcept { return m_bIsInitialized; }
	void Reset();

private:
	float m_clearDepth{ 1.0f };
	uint8_t m_clearStencil{ 0 };
	bool m_bIsInitialized{ false };
};

} // namespace Luna