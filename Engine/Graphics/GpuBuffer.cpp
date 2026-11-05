//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#include "Stdafx.h"

#include "GpuBuffer.h"

#include "Device.h"

using namespace std;


namespace Luna
{

GpuBufferPtr CreateConstantBuffer(const string& name, size_t elementCount, size_t elementSize, const void* initialData)
{
	GpuBufferDesc desc{
		.name			= name,
		.resourceType	= ResourceType::ConstantBuffer,
		.memoryAccess	= MemoryAccess::GpuRead | MemoryAccess::CpuWrite,
		.elementCount	= elementCount,
		.elementSize	= elementSize,
		.initialData	= initialData
	};

	return GetDevice()->CreateGpuBuffer(desc);
}


GpuBufferPtr CreateIndexBuffer(const string& name, std::span<uint16_t> indexData)
{
	GpuBufferDesc desc{
		.name			= name,
		.resourceType	= ResourceType::IndexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= indexData.size(),
		.elementSize	= sizeof(uint16_t),
		.initialData	= indexData.data()
	};

	return GetDevice()->CreateGpuBuffer(desc);
}


GpuBufferPtr CreateIndexBuffer(const string& name, std::span<uint32_t> indexData)
{
	GpuBufferDesc desc{
		.name			= name,
		.resourceType	= ResourceType::IndexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= indexData.size(),
		.elementSize	= sizeof(uint32_t),
		.initialData	= indexData.data()
	};

	return GetDevice()->CreateGpuBuffer(desc);
}


GpuBufferPtr CreateVertexBuffer(const string& name, size_t elementCount, size_t elementSize, const void* initialData)
{
	GpuBufferDesc desc{
		.name			= name,
		.resourceType	= ResourceType::VertexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= elementCount,
		.elementSize	= elementSize,
		.initialData	= initialData
	};

	return GetDevice()->CreateGpuBuffer(desc);
}


GpuBufferPtr CreateDrawIndirectArgsBuffer(const std::string& name, size_t elementCount, const void* initialData)
{
	GpuBufferDesc desc{
		.name			= name,
		.resourceType	= ResourceType::IndirectArgsBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= elementCount,
		.elementSize	= sizeof(DrawIndirectArgs),
		.initialData	= initialData
	};
	return GetDevice()->CreateGpuBuffer(desc);
}


GpuBufferPtr CreateDrawIndexedIndirectArgsBuffer(const std::string& name, size_t elementCount, const void* initialData)
{
	GpuBufferDesc desc{
		.name			= name,
		.resourceType	= ResourceType::IndirectArgsBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= elementCount,
		.elementSize	= sizeof(DrawIndexedIndirectArgs),
		.initialData	= initialData
	};
	return GetDevice()->CreateGpuBuffer(desc);
}


GpuBufferPtr CreateDispatchIndirectArgsBuffer(const std::string& name, size_t elementCount, const void* initialData)
{ 
	GpuBufferDesc desc{
		.name = name,
		.resourceType	= ResourceType::IndirectArgsBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= elementCount,
		.elementSize	= sizeof(DispatchIndirectArgs),
		.initialData	= initialData
	};
	return GetDevice()->CreateGpuBuffer(desc);
}

} // namespace Luna