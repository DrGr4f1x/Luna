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

#include "Graphics\GpuBuffer.h"


namespace Luna
{

struct RayTracingGeometryTrianglesDesc
{
	uint64_t transform3x4{ 0 };
	Format indexFormat{ Format::R32_UInt };
	Format vertexFormat{ Format::RGB32_Float };
	size_t indexCount{ 0 };
	size_t vertexCount{ 0 };
	size_t vertexStride{ 0 };
	uint64_t indexBuffer{ 0 };
	uint64_t vertexBuffer{ 0 };
};


struct RayTracingGeometryAABBsDesc
{
	uint64_t aabbCount{ 0 };
	uint64_t aabbStride{ 0 };
	uint64_t aabbBuffer{ 0 };
};


struct RayTracingGeometryOmmTrianglesDesc
{
	// TODO
};


struct RayTracingGeometryDesc
{
	std::string name;
	RayTracingGeometryType geometryType{ RayTracingGeometryType::Triangles };
	RayTracingGeometryFlags flags{ RayTracingGeometryFlags::None };
	RayTracingAccelerationStructureFlags buildFlags{ RayTracingAccelerationStructureFlags::None };
	union
	{
		RayTracingGeometryTrianglesDesc triangles;
		RayTracingGeometryAABBsDesc aabbs;
		RayTracingGeometryOmmTrianglesDesc omm;
	};
};


struct RayTracingBuildInfoBase
{
	GpuBufferPtr scratchBuffer;
	GpuBufferPtr destBuffer;
	RayTracingAccelerationStructureType type{};
	RayTracingAccelerationStructureFlags flags{};
};


struct RayTracingInstanceDesc
{
	GpuBufferPtr blas;
	Math::Matrix4 transform{ Math::kIdentity };
	uint32_t instanceID;
	uint32_t instanceMask;
};


struct RayTracingTLASDesc
{
	std::span<RayTracingInstanceDesc> instances;
	RayTracingAccelerationStructureFlags buildFlags{ RayTracingAccelerationStructureFlags::None };
};

} // namespace Luna