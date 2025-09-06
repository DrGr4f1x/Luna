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
#include "Graphics\InputLayout.h"

#include "DirectXCollision.h"


// Forward declarations
namespace Luna
{
class IDevice;
} // namespace Luna


struct Subset
{
	uint32_t offset{ 0 };
	uint32_t count{ 0 };
};


struct MeshInfo
{
	uint32_t indexSize{ 0 };
	uint32_t meshletCount{ 0 };

	uint32_t lastMeshletVertCount{ 0 };
	uint32_t lastMeshletPrimCount{ 0 };
};


struct Meshlet
{
	uint32_t vertCount{ 0 };
	uint32_t vertOffset{ 0 };
	uint32_t primCount{ 0 };
	uint32_t primOffset{ 0 };
};


struct PackedTriangle
{
	uint32_t i0 : 10;
	uint32_t i1 : 10;
	uint32_t i2 : 10;
};


struct CullData
{
	DirectX::BoundingSphere boundingSphere; // xyz = center, w = radius
	uint8_t normalCone[4];  // xyz = axis, w = -cos(a + 90)
	float apexOffset;     // apex = center - axis * offset
};


struct MeshletMesh
{
	std::vector<Luna::VertexElementDesc> layoutElems;

	std::vector<std::span<std::byte>> vertices;
	std::vector<uint32_t> vertexStrides;
	uint32_t vertexCount;
	DirectX::BoundingSphere boundingSphere;

	std::span<Subset> indexSubsets;
	std::span<std::byte> indices;
	uint32_t indexSize;
	uint32_t indexCount;

	std::span<Subset> meshletSubsets;
	std::span<Meshlet> meshlets;
	std::span<uint8_t> uniqueVertexIndices;
	std::span<PackedTriangle> primitiveIndices;
	std::span<CullData> cullingData;

	std::vector<Luna::GpuBufferPtr> vertexResources;
	Luna::GpuBufferPtr indexResource;
	Luna::GpuBufferPtr meshletResource;
	Luna::GpuBufferPtr uniqueVertexIndexResource;
	Luna::GpuBufferPtr primitiveIndexResource;
	Luna::GpuBufferPtr cullDataResource; // TODO: Get rid of this?  I don't think it's used.
	Luna::GpuBufferPtr meshInfoResource;

	// Calculates the number of instances of the last meshlet which can be packed into a single threadgroup.
	uint32_t GetLastMeshletPackCount(uint32_t subsetIndex, uint32_t maxGroupVerts, uint32_t maxGroupPrims)
	{
		if (meshlets.size() == 0)
		{
			return 0;
		}

		auto& subset = meshletSubsets[subsetIndex];
		auto& meshlet = meshlets[subset.offset + subset.count - 1];

		return std::min(maxGroupVerts / meshlet.vertCount, maxGroupPrims / meshlet.primCount);
	}


	void GetPrimitive(uint32_t index, uint32_t& i0, uint32_t& i1, uint32_t& i2) const
	{
		auto prim = primitiveIndices[index];
		i0 = prim.i0;
		i1 = prim.i1;
		i2 = prim.i2;
	}


	uint32_t GetVertexIndex(uint32_t index) const
	{
		const uint8_t* addr = uniqueVertexIndices.data() + index * indexSize;
		if (indexSize == 4)
		{
			return *reinterpret_cast<const uint32_t*>(addr);
		}
		else
		{
			return *reinterpret_cast<const uint16_t*>(addr);
		}
	}
};


class MeshletModel
{
public:
	HRESULT LoadFromFile(const std::string& filename);
	HRESULT InitResources(Luna::IDevice* device);

	uint32_t GetMeshCount() const { return static_cast<uint32_t>(m_meshes.size()); }
	const MeshletMesh& GetMesh(uint32_t i) const { return m_meshes[i]; }

	const DirectX::BoundingSphere& GetBoundingSphere() const { return m_boundingSphere; }

	// Iterator interface
	auto begin() { return m_meshes.begin(); }
	auto end() { return m_meshes.end(); }

private:
	std::vector<MeshletMesh> m_meshes;
	DirectX::BoundingSphere m_boundingSphere;

	std::vector<std::byte> m_buffer;
};