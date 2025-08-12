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

// Forward declarations
class IDescriptor;


struct GpuBufferDesc
{
	std::string name;
	ResourceType resourceType{ ResourceType::Unknown };
	MemoryAccess memoryAccess{ MemoryAccess::Unknown };
	Format format{ Format::Unknown };
	size_t elementCount{ 0 };
	size_t elementSize{ 0 };
	const void* initialData{ nullptr };
	bool bAllowShaderResource{ false };
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


class IGpuBuffer : public IGpuResource
{
public:
	Format GetFormat() const noexcept { return m_format; }
	size_t GetBufferSize() const noexcept { return m_bufferSize; }
	size_t GetElementSize() const noexcept { return m_elementSize; }
	size_t GetElementCount() const noexcept { return m_elementCount; }

	virtual void Update(size_t sizeInBytes, const void* data) = 0;
	virtual void Update(size_t sizeInBytes, size_t offset, const void* data) = 0;

	virtual void* Map() = 0;
	virtual void Unmap() = 0;

	virtual const IDescriptor* GetSrvDescriptor() const noexcept = 0;
	virtual const IDescriptor* GetUavDescriptor() const noexcept = 0;
	virtual const IDescriptor* GetCbvDescriptor() const noexcept = 0;

protected:
	Format m_format{ Format::Unknown };
	size_t m_bufferSize{ 0 };
	size_t m_elementSize{ 0 };
	size_t m_elementCount{ 0 };
};

using GpuBufferPtr = std::shared_ptr<IGpuBuffer>;


// Indirect argument buffer payloads
struct DrawIndirectArgs
{
	uint32_t vertexCountPerInstance;
	uint32_t instanceCount;
	uint32_t startVertexLocation;
	uint32_t startInstanceLocation;
};


struct DrawIndexedIndirectArgs
{
	uint32_t indexCountPerInstance;
	uint32_t instanceCount;
	uint32_t startIndexLocation;
	uint32_t baseVertexLocation;
	uint32_t startInstanceLocation;
};


struct DispatchIndirectArgs
{
	uint32_t threadGroupCountX;
	uint32_t threadGroupCountY;
	uint32_t threadGroupCountZ;
};


// Helper functions for describing and creating basic GpuBuffer types
GpuBufferDesc DescribeConstantBuffer(const std::string& name, size_t elementCount, size_t elementSize);
GpuBufferPtr CreateConstantBuffer(const std::string& name, size_t elementCount, size_t elementSize, const void* initialData = nullptr);

GpuBufferDesc DescribeIndexBuffer(const std::string& name, size_t elementCount, size_t elementSize);
GpuBufferPtr CreateIndexBuffer(const std::string& name, std::span<uint16_t> indexData);
GpuBufferPtr CreateIndexBuffer(const std::string& name, std::span<uint32_t> indexData);

GpuBufferDesc DescribeVertexBuffer(const std::string& name, size_t elementCount, size_t elementSize);
GpuBufferPtr CreateVertexBuffer(const std::string& name, size_t elementCount, size_t elementSize, const void* initialData = nullptr);

template <typename T>
GpuBufferPtr CreateVertexBuffer(const std::string& name, std::span<T> vertexData)
{
	return CreateVertexBuffer(name, vertexData.size(), sizeof(T), vertexData.data());
}

GpuBufferDesc DescribeDrawIndirectArgsBuffer(const std::string& name, size_t elementCount);
GpuBufferPtr CreateDrawIndirectArgsBuffer(const std::string& name, size_t elementCount, const void* initialData = nullptr);

GpuBufferDesc DescribeDrawIndexedIndirectArgsBuffer(const std::string& name, size_t elementCount);
GpuBufferPtr CreateDrawIndexedIndirectArgsBuffer(const std::string& name, size_t elementCount, const void* initialData = nullptr);

GpuBufferDesc DescribeDispatchIndirectArgsBuffer(const std::string& name, size_t elementCount);
GpuBufferPtr CreateDispatchIndirectArgsBuffer(const std::string& name, size_t elementCount, const void* initialData = nullptr);

} // namespace Luna