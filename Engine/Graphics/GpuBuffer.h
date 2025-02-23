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


class IGpuBufferManager;


class __declspec(uuid("3BC9117D-D567-4D21-9AFC-ACFF5D17C167")) GpuBufferHandleType : public RefCounted<GpuBufferHandleType>
{
public:
	GpuBufferHandleType(uint32_t index, IGpuBufferManager* manager)
		: m_index{ index }
		, m_manager{ manager }
	{}
	~GpuBufferHandleType();

	uint32_t GetIndex() const { return m_index; }

private:
	uint32_t m_index{ 0 };
	IGpuBufferManager* m_manager{ nullptr };
};

using GpuBufferHandle = wil::com_ptr<GpuBufferHandleType>;


class IGpuBufferManager
{
public:
	// Create/Destroy GpuBuffer
	virtual GpuBufferHandle CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc) = 0;
	virtual void DestroyHandle(GpuBufferHandleType* handle) = 0;

	// Platform agnostic functions
	virtual ResourceType GetResourceType(GpuBufferHandleType* handle) const = 0;
	virtual ResourceState GetUsageState(GpuBufferHandleType* handle) const = 0;
	virtual void SetUsageState(GpuBufferHandleType* handle, ResourceState newState) = 0;
	virtual size_t GetSize(GpuBufferHandleType* handle) const = 0;
	virtual size_t GetElementCount(GpuBufferHandleType* handle) const = 0;
	virtual size_t GetElementSize(GpuBufferHandleType* handle) const = 0;
	virtual void Update(GpuBufferHandleType* handle, size_t sizeInBytes, size_t offset, const void* data) const = 0;

};


class GpuBuffer
{
public:

	void Initialize(const GpuBufferDesc& gpuBufferDesc);

	GpuBufferHandle GetHandle() const { return m_handle; }

	ResourceType GetResourceType() const;
	ResourceState GetUsageState() const;
	void SetUsageState(ResourceState newState);

	size_t GetSize() const;
	size_t GetElementCount() const;
	size_t GetElementSize() const;
	// TODO: Sweep the code for this stuff and use std::span (or a view?)
	void Update(size_t sizeInBytes, const void* data);
	void Update(size_t sizeInBytes, size_t offset, const void* data);

private:
	GpuBufferHandle m_handle;
};

} // namespace Luna