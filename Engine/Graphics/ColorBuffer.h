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
#include "Graphics\PixelBuffer.h"


namespace Luna
{

struct ColorBufferDesc
{
	std::string name;
	ResourceType resourceType{ ResourceType::Texture2D };
	uint64_t width{ 0 };
	uint32_t height{ 0 };
	uint32_t arraySizeOrDepth{ 1 };
	uint32_t numMips{ 1 };
	uint32_t numSamples{ 1 };
	Format format{ Format::Unknown };
	uint32_t numFragments{ 1 };
	uint8_t planeCount{ 1 };
	Color clearColor{ DirectX::Colors::Black };

	ColorBufferDesc& SetName(const std::string& value) { name = value; return *this; }
	constexpr ColorBufferDesc& SetResourceType(ResourceType value) noexcept { resourceType = value; return *this; }
	constexpr ColorBufferDesc& SetWidth(uint64_t value) noexcept { width = value; return *this; }
	constexpr ColorBufferDesc& SetHeight(uint32_t value) noexcept { height = value; return *this; }
	constexpr ColorBufferDesc& SetArraySize(uint32_t value) noexcept { arraySizeOrDepth = value; return *this; }
	constexpr ColorBufferDesc& SetDepth(uint32_t value) noexcept { arraySizeOrDepth = value; return *this; }
	constexpr ColorBufferDesc& SetNumMips(uint32_t value) noexcept { numMips = value; return *this; }
	constexpr ColorBufferDesc& SetNumSamples(uint32_t value) noexcept { numSamples = value; return *this; }
	constexpr ColorBufferDesc& SetFormat(Format value) noexcept { format = value; return *this; }
	constexpr ColorBufferDesc& SetNumFragments(uint32_t value) noexcept { numFragments = value; return *this; }
	constexpr ColorBufferDesc& SetPlaneCount(uint8_t value) noexcept { planeCount = value; return *this; }
	ColorBufferDesc& SetClearColor(Color value) noexcept { clearColor = value; return *this; }
};


class ColorBuffer : public PixelBuffer
{
public:
	void Initialize(const ColorBufferDesc& colorBufferDesc);

	Color GetClearColor() const;

	void SetHandle(ResourceHandle handle);
};


class ColorBufferFactoryBase
{
protected:
	static const uint32_t MaxResources = (1 << 10);
	static const uint32_t InvalidAllocation = ~0u;

public:
	ColorBufferFactoryBase()
	{
		ClearDescs();
	}

	Format GetFormat(uint32_t index) const { return m_descs[index].format; }

	uint64_t GetWidth(uint32_t index) const { return m_descs[index].width; }
	uint32_t GetHeight(uint32_t index) const { return m_descs[index].height; }
	uint32_t GetDepthOrArraySize(uint32_t index) const { return m_descs[index].arraySizeOrDepth; }
	uint32_t GetNumMips(uint32_t index) const { return m_descs[index].numMips; }
	uint32_t GetNumSamples(uint32_t index) const { return m_descs[index].numSamples; }
	uint32_t GetPlaneCount(uint32_t index) const { return m_descs[index].planeCount; }

	Color GetClearColor(uint32_t index) const { return m_descs[index].clearColor; }

protected:
	void ResetDesc(uint32_t index)
	{
		m_descs[index] = ColorBufferDesc{};
	}

	void ClearDescs()
	{
		for (uint32_t i = 0; i < MaxResources; ++i)
		{
			ResetDesc(i);
		}
	}

protected:
	std::array<ColorBufferDesc, MaxResources> m_descs;
};

} // namespace Luna