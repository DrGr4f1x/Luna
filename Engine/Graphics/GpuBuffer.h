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
#include "Graphics\GraphicsCommon.h"


namespace Luna
{

struct GpuBufferDesc
{
	std::string name;
	ResourceType resourceType{ ResourceType::Unknown };
	MemoryAccess memoryAccess{ MemoryAccess::Unknown };
	Format format{ Format::Unknown };
	size_t elementCount{ 0 };
	size_t elementSize{ 0 };
	const void* initialData{ nullptr };
	bool bAllowUnorderedAccess{ false };

	GpuBufferDesc& SetName(const std::string& value) { name = value; return *this; }
	constexpr GpuBufferDesc& SetResourceType(ResourceType value) noexcept { resourceType = value; return *this; }
	constexpr GpuBufferDesc& SetMemoryAccess(MemoryAccess value) noexcept { memoryAccess = value; return *this; }
	constexpr GpuBufferDesc& SetFormat(Format value) noexcept { format = value; return *this; }
	constexpr GpuBufferDesc& SetElementCount(size_t value) noexcept { elementCount = value; return *this; }
	constexpr GpuBufferDesc& SetElementSize(size_t value) noexcept { elementSize = value; return *this; }
	constexpr GpuBufferDesc& SetInitialData(const void* data) noexcept { initialData = data; return *this; }
	constexpr GpuBufferDesc& SetAllowUnorderedAccess(bool value) noexcept { bAllowUnorderedAccess = value; return *this; }
};


class GpuBuffer : public GpuResource
{
public:
	void Initialize(const GpuBufferDesc& gpuBufferDesc);

	size_t GetSize() const;
	size_t GetElementCount() const;
	size_t GetElementSize() const;
	// TODO: Sweep the code for this stuff and use std::span (or a view?)
	void Update(size_t sizeInBytes, const void* data);
	void Update(size_t sizeInBytes, size_t offset, const void* data);
};


class GpuBufferFactoryBase
{
protected:
	static const uint32_t MaxResources = (1 << 10);
	static const uint32_t InvalidAllocation = ~0u;

public:
	GpuBufferFactoryBase()
	{
		ClearDescs();
	}

	Format GetFormat(uint32_t index) const { return m_descs[index].format; }
	size_t GetSize(uint32_t index) const { return m_descs[index].elementCount * m_descs[index].elementSize; }
	size_t GetElementCount(uint32_t index) const { return m_descs[index].elementCount; }
	size_t GetElementSize(uint32_t index) const { return m_descs[index].elementSize; }

protected:
	void ResetDesc(uint32_t index)
	{
		m_descs[index] = GpuBufferDesc{};
	}

	void ClearDescs()
	{
		for (uint32_t i = 0; i < MaxResources; ++i)
		{
			ResetDesc(i);
		}
	}

protected:
	std::array<GpuBufferDesc, MaxResources> m_descs;
};

} // namespace Luna