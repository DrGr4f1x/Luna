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
#include "Graphics\GpuResource.h"


namespace Luna
{

struct GpuBufferDesc
{
	std::string name;
	ResourceType resourceType;
	MemoryAccess memoryAccess;
	size_t elementCount{ 0 };
	size_t elementSize{ 0 };
	const void* initialData{ nullptr };

	GpuBufferDesc& SetName(const std::string& value) { name = value; return *this; }
	constexpr GpuBufferDesc& SetResourceType(ResourceType value) noexcept { resourceType = value; return *this; }
	constexpr GpuBufferDesc& SetMemoryAccess(MemoryAccess value) noexcept { memoryAccess = value; return *this; }
	constexpr GpuBufferDesc& SetElementCount(size_t value) noexcept { elementCount = value; return *this; }
	constexpr GpuBufferDesc& SetElementSize(size_t value) noexcept { elementSize = value; return *this; }
	constexpr GpuBufferDesc& SetInitialData(const void* data) { initialData = data; return *this; }
};


class GpuBuffer : public GpuResource
{
public:
	size_t GetSize() const noexcept { return m_elementCount * m_elementSize; }
	size_t GetElementCount() const noexcept { return m_elementCount; }
	size_t GetElementSize() const noexcept { return m_elementSize; }

	bool Initialize(GpuBufferDesc& desc);
	bool IsInitialized() const noexcept { return m_bIsInitialized; }
	void Reset();

protected:
	explicit GpuBuffer(ResourceType resourceType) noexcept
	{
		m_resourceType = resourceType;
	}

protected:
	size_t m_elementCount{ 0 };
	size_t m_elementSize{ 0 };
	bool m_bIsInitialized{ false };
};


class IndexBuffer : public GpuBuffer
{
public:
	IndexBuffer();

	bool IsIndex16Bit() const noexcept { return (m_elementSize == 2); }
};

} // namespace Luna