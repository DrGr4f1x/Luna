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

struct DepthBufferDesc
{
	std::string name;
	ResourceType resourceType{ ResourceType::Texture2D };
	uint64_t width{ 0 };
	uint32_t height{ 0 };
	uint32_t arraySizeOrDepth{ 1 };
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
	void Initialize(const DepthBufferDesc& depthBufferDesc);

	float GetClearDepth() const;
	uint8_t GetClearStencil() const;
};


class DepthBufferFactoryBase
{
protected:
	static const uint32_t MaxResources = (1 << 8);
	static const uint32_t InvalidAllocation = ~0u;

public:
	DepthBufferFactoryBase()
	{
		ClearDescs();
	}

	Format GetFormat(uint32_t index) const { return m_descs[index].format; }

	uint64_t GetWidth(uint32_t index) const { return m_descs[index].width; }
	uint32_t GetHeight(uint32_t index) const { return m_descs[index].height; }
	uint32_t GetDepthOrArraySize(uint32_t index) const { return m_descs[index].arraySizeOrDepth; }
	uint32_t GetNumMips(uint32_t index) const { return m_descs[index].numMips; }
	uint32_t GetNumSamples(uint32_t index) const { return m_descs[index].numSamples; }
	uint32_t GetPlaneCount(uint32_t index) const { return 1; /* TODO: return a real value here */ }

	float GetClearDepth(uint32_t index) const { return m_descs[index].clearDepth; }
	uint8_t GetClearStencil(uint32_t index) const { return m_descs[index].clearStencil; }

protected:
	void ResetDesc(uint32_t index)
	{
		m_descs[index] = DepthBufferDesc{};
	}

	void ClearDescs()
	{
		for (uint32_t i = 0; i < MaxResources; ++i)
		{
			ResetDesc(i);
		}
	}

protected:
	std::array<DepthBufferDesc, MaxResources> m_descs;
};

} // namespace Luna