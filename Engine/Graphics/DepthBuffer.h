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


class IDepthBufferPool;


class __declspec(uuid("BFA0209B-629D-4A01-9E6E-5A4D2F8D884D")) DepthBufferHandleType : public RefCounted<DepthBufferHandleType>
{
public:
	DepthBufferHandleType(uint32_t index, IDepthBufferPool* pool)
		: m_index{ index }
		, m_pool{ pool }
	{}
	~DepthBufferHandleType();

	uint32_t GetIndex() const { return m_index; }

private:
	uint32_t m_index{ 0 };
	IDepthBufferPool* m_pool{ nullptr };
};

using DepthBufferHandle = wil::com_ptr<DepthBufferHandleType>;


class IDepthBufferPool
{
public:
	// Create/Destroy DepthBuffer
	virtual DepthBufferHandle CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc) = 0;
	virtual void DestroyHandle(DepthBufferHandleType* handle) = 0;

	// Platform agnostic functions
	virtual ResourceType GetResourceType(DepthBufferHandleType* handle) const = 0;
	virtual ResourceState GetUsageState(DepthBufferHandleType* handle) const = 0;
	virtual void SetUsageState(DepthBufferHandleType* handle, ResourceState newState) = 0;
	virtual uint64_t GetWidth(DepthBufferHandleType* handle) const = 0;
	virtual uint32_t GetHeight(DepthBufferHandleType* handle) const = 0;
	virtual uint32_t GetDepthOrArraySize(DepthBufferHandleType* handle) const = 0;
	virtual uint32_t GetNumMips(DepthBufferHandleType* handle) const = 0;
	virtual uint32_t GetNumSamples(DepthBufferHandleType* handle) const = 0;
	virtual uint32_t GetPlaneCount(DepthBufferHandleType* handle) const = 0;
	virtual Format GetFormat(DepthBufferHandleType* handle) const = 0;
	virtual float GetClearDepth(DepthBufferHandleType* handle) const = 0;
	virtual uint8_t GetClearStencil(DepthBufferHandleType* handle) const = 0;
};


class DepthBuffer
{
public:
	void Initialize(const DepthBufferDesc& depthBufferDesc);

	ResourceType GetResourceType() const;
	ResourceState GetUsageState() const;
	void SetUsageState(ResourceState newState);
	uint64_t GetWidth() const;
	uint32_t GetHeight() const;
	uint32_t GetDepth() const;
	uint32_t GetArraySize() const;
	uint32_t GetNumMips() const;
	uint32_t GetNumSamples() const;
	uint32_t GetPlaneCount() const;
	Format GetFormat() const;
	TextureDimension GetDimension() const;
	float GetClearDepth() const;
	uint8_t GetClearStencil() const;

	DepthBufferHandle GetHandle() const { return m_handle; }

private:
	DepthBufferHandle m_handle;
};

} // namespace Luna