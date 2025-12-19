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

#include "MeshletModel.h"

#include "BinaryReader.h"
#include "FileSystem.h"
#include "Graphics\Device.h"

using namespace Luna;
using namespace Math;
using namespace std;


namespace
{

vector<VertexElementDesc> c_vertexElements{
	{ "POSITION", 0, Format::RGB32_Float, 0, APPEND_ALIGNED_ELEMENT, InputClassification::PerVertexData, 1 },
	{ "NORMAL", 0, Format::RGB32_Float, 0, APPEND_ALIGNED_ELEMENT, InputClassification::PerVertexData, 1 },
	{ "TEXCOORD", 0, Format::RG32_Float, 0, APPEND_ALIGNED_ELEMENT, InputClassification::PerVertexData, 1 },
	{ "TANGENT", 0, Format::RGB32_Float, 0, APPEND_ALIGNED_ELEMENT, InputClassification::PerVertexData, 1 },
	{ "BITANGENT", 0, Format::RGB32_Float, 0, APPEND_ALIGNED_ELEMENT, InputClassification::PerVertexData, 1 }
};

const uint32_t c_prolog = 'MSHL';


enum FileVersion
{
	FILE_VERSION_INITIAL = 0,
	CURRENT_FILE_VERSION = FILE_VERSION_INITIAL
};


struct FileHeader
{
	uint32_t prolog;
	uint32_t version;

	uint32_t meshCount;
	uint32_t accessorCount;
	uint32_t bufferViewCount;
	uint32_t bufferSize;
};


struct MeshHeader
{
	uint32_t indices;
	uint32_t indexSubsets;
	uint32_t attributes[5];

	uint32_t meshlets;
	uint32_t meshletSubsets;
	uint32_t uniqueVertexIndices;
	uint32_t primitiveIndices;
	uint32_t cullData;
};


struct BufferView
{
	uint32_t offset;
	uint32_t size;
};


struct Accessor
{
	uint32_t bufferView;
	uint32_t offset;
	uint32_t size;
	uint32_t stride;
	uint32_t count;
};

} // anonymous namespace


HRESULT MeshletModel::LoadFromFile(const string& filename)
{
	string filepath = GetFileSystem()->GetFullPath(filename);

	size_t dataSize = 0;
	unique_ptr<std::byte[]> data;
	BinaryReader::ReadEntireFile(filepath, data, &dataSize);

	vector<MeshHeader> meshes;
	vector<BufferView> bufferViews;
	vector<Accessor> accessors;

	std::byte* dataptr = data.get();
	FileHeader* header = (FileHeader*)dataptr;
	dataptr += sizeof(FileHeader);

	if (header->prolog != c_prolog)
	{
		return E_FAIL; // Incorrect file format.
	}

	if (header->version != CURRENT_FILE_VERSION)
	{
		return E_FAIL; // Version mismatch between export and import serialization code.
	}

	// Read mesh metadata
	meshes.resize(header->meshCount);
	memcpy(meshes.data(), dataptr, meshes.size() * sizeof(meshes[0]));
	dataptr += meshes.size() * sizeof(meshes[0]);

	accessors.resize(header->accessorCount);
	memcpy(accessors.data(), dataptr, accessors.size() * sizeof(accessors[0]));
	dataptr += accessors.size() * sizeof(accessors[0]);

	bufferViews.resize(header->bufferViewCount);
	memcpy(bufferViews.data(), dataptr, bufferViews.size() * sizeof(bufferViews[0]));
	dataptr += bufferViews.size() * sizeof(bufferViews[0]);

	m_buffer.resize(header->bufferSize);
	memcpy(m_buffer.data(), dataptr, header->bufferSize);

	// Populate mesh data from binary data and metadata
	m_meshes.resize(meshes.size());
	for (uint32_t i = 0; i < (uint32_t)meshes.size(); ++i)
	{
		const auto& meshView = meshes[i];
		auto& mesh = m_meshes[i];

		// Index data
		{
			const Accessor& accessor = accessors[meshView.indices];
			const BufferView& bufferView = bufferViews[accessor.bufferView];

			mesh.indexSize = accessor.size;
			mesh.indexCount = accessor.count;

			mesh.indices = span<std::byte>(m_buffer.data() + bufferView.offset, bufferView.size);
		}

		// Index Subset data
		{
			const Accessor& accessor = accessors[meshView.indexSubsets];
			const BufferView& bufferView = bufferViews[accessor.bufferView];

			mesh.indexSubsets = span<Subset>(reinterpret_cast<Subset*>(m_buffer.data() + bufferView.offset), accessor.count);
		}

		// Vertex data & layout metadata

		// Determine the number of unique Buffer Views associated with the vertex attributes & copy vertex buffers.
		std::vector<uint32_t> vbMap;

		for (uint32_t j = 0; j < 5; ++j)
		{
			if (meshView.attributes[j] == -1)
				continue;

			const Accessor& accessor = accessors[meshView.attributes[j]];

			auto it = std::find(vbMap.begin(), vbMap.end(), accessor.bufferView);
			if (it != vbMap.end())
			{
				continue; // Already added - continue.
			}

			// New buffer view encountered; add to list and copy vertex data
			vbMap.push_back(accessor.bufferView);
			const BufferView& bufferView = bufferViews[accessor.bufferView];

			span<std::byte> verts = { m_buffer.data() + bufferView.offset, bufferView.size };

			mesh.vertexStrides.push_back(accessor.stride);
			mesh.vertices.push_back(verts);
			mesh.vertexCount = static_cast<uint32_t>(verts.size()) / accessor.stride;
		}

		// Populate the vertex buffer metadata from accessors.
		for (uint32_t j = 0; j < 5; ++j)
		{
			if (meshView.attributes[j] == -1)
				continue;

			const Accessor& accessor = accessors[meshView.attributes[j]];

			// Determine which vertex buffer index holds this attribute's data
			auto it = std::find(vbMap.begin(), vbMap.end(), accessor.bufferView);

			VertexElementDesc desc = c_vertexElements[j];
			desc.inputSlot = static_cast<uint32_t>(std::distance(vbMap.begin(), it));

			mesh.layoutElems.push_back(desc);
		}

		// Meshlet data
		{
			const Accessor& accessor = accessors[meshView.meshlets];
			const BufferView& bufferView = bufferViews[accessor.bufferView];

			mesh.meshlets = { reinterpret_cast<Meshlet*>(m_buffer.data() + bufferView.offset), accessor.count };
		}

		// Meshlet Subset data
		{
			const Accessor& accessor = accessors[meshView.meshletSubsets];
			const BufferView& bufferView = bufferViews[accessor.bufferView];

			mesh.meshletSubsets = { reinterpret_cast<Subset*>(m_buffer.data() + bufferView.offset), accessor.count };
		}

		// Unique Vertex Index data
		{
			const Accessor& accessor = accessors[meshView.uniqueVertexIndices];
			const BufferView& bufferView = bufferViews[accessor.bufferView];

			mesh.uniqueVertexIndices = { reinterpret_cast<uint8_t*>(m_buffer.data() + bufferView.offset), bufferView.size };
		}

		// Primitive Index data
		{
			const Accessor& accessor = accessors[meshView.primitiveIndices];
			const BufferView& bufferView = bufferViews[accessor.bufferView];

			mesh.primitiveIndices = { reinterpret_cast<PackedTriangle*>(m_buffer.data() + bufferView.offset), accessor.count };
		}

		// Cull data
		{
			const Accessor& accessor = accessors[meshView.cullData];
			const BufferView& bufferView = bufferViews[accessor.bufferView];

			mesh.cullingData = { reinterpret_cast<CullData*>(m_buffer.data() + bufferView.offset), accessor.count };
		}
	}

	// Build bounding spheres for each mesh
	for (uint32_t i = 0; i < static_cast<uint32_t>(m_meshes.size()); ++i)
	{
		auto& m = m_meshes[i];

		uint32_t vbIndexPos = 0;

		// Find the index of the vertex buffer of the position attribute
		for (uint32_t j = 1; (uint32_t)j < m.layoutElems.size(); ++j)
		{
			auto& desc = m.layoutElems[j];
			if (strcmp(desc.semanticName, "POSITION") == 0)
			{
				vbIndexPos = j;
				break;
			}
		}

		// Find the byte offset of the position attribute with its vertex buffer
		uint32_t positionOffset = 0;

		for (uint32_t j = 0; (uint32_t)j < m.layoutElems.size(); ++j)
		{
			auto& desc = m.layoutElems[j];
			if (strcmp(desc.semanticName, "POSITION") == 0)
			{
				break;
			}

			if (desc.inputSlot == vbIndexPos)
			{
				positionOffset += BlockSize(m.layoutElems[j].format);
			}
		}

		XMFLOAT3* v0 = reinterpret_cast<XMFLOAT3*>(m.vertices[vbIndexPos].data() + positionOffset);
		uint32_t stride = m.vertexStrides[vbIndexPos];

		DirectX::BoundingSphere::CreateFromPoints(m.boundingSphere, m.vertexCount, v0, stride);

		if (i == 0)
		{
			m_boundingSphere = m.boundingSphere;
		}
		else
		{
			DirectX::BoundingSphere::CreateMerged(m_boundingSphere, m_boundingSphere, m.boundingSphere);
		}
	}
	
	return S_OK;
}


HRESULT MeshletModel::InitResources(IDevice* device)
{
	// Create GpuBuffers
	for (uint32_t i = 0; i < (uint32_t)m_meshes.size(); ++i)
	{
		auto& m = m_meshes[i];

		// Vertex resources
		m.vertexResources.resize(m.vertices.size());
		for (uint32_t j = 0; j < (uint32_t)m.vertices.size(); ++j)
		{
			GpuBufferDesc desc{
				.name			= format("Mesh {} - Vertex Resource {}", i, j),
				.resourceType	= ResourceType::StructuredBuffer,
				.memoryAccess	= MemoryAccess::GpuRead,
				.elementCount	= m.vertices[j].size() / m.vertexStrides[j],
				.elementSize	= m.vertexStrides[j],
				.initialData	= m.vertices[j].data()
			};
			m.vertexResources[j] = device->CreateGpuBuffer(desc);
		}

		// Index resource
		{
			GpuBufferDesc desc{
				.name			= format("Mesh {} - Index Resource", i),
				.resourceType	= ResourceType::StructuredBuffer,
				.memoryAccess	= MemoryAccess::GpuRead,
				.elementCount	= m.indices.size() / m.indexSize,
				.elementSize	= m.indexSize,
				.initialData	= m.indices.data()
			};
			m.indexResource = device->CreateGpuBuffer(desc);
		}

		// Meshlet resource
		{
			GpuBufferDesc desc{
				.name			= format("Mesh {} - Meshlet Resource", i),
				.resourceType	= ResourceType::StructuredBuffer,
				.memoryAccess	= MemoryAccess::GpuRead,
				.elementCount	= m.meshlets.size(),
				.elementSize	= sizeof(Meshlet),
				.initialData	= m.meshlets.data()
			};
			m.meshletResource = device->CreateGpuBuffer(desc);
		}

		// UniqueVertexIndex resource
		{
			GpuBufferDesc desc{
				.name			= format("Mesh {} - UniqueVertexIndex Resource", i),
				.resourceType	= ResourceType::ByteAddressBuffer,
				.memoryAccess	= MemoryAccess::GpuRead,
				.elementCount	= m.uniqueVertexIndices.size(),
				.elementSize	= sizeof(uint8_t),
				.initialData	= m.uniqueVertexIndices.data()
			};
			m.uniqueVertexIndexResource = device->CreateGpuBuffer(desc);
		}

		// PrimitiveIndex resource
		{
			GpuBufferDesc desc{
				.name			= format("Mesh {} - PrimitiveIndex Resource", i),
				.resourceType	= ResourceType::StructuredBuffer,
				.memoryAccess	= MemoryAccess::GpuRead,
				.elementCount	= m.primitiveIndices.size(),
				.elementSize	= sizeof(PackedTriangle),
				.initialData	= m.primitiveIndices.data()
			};
			m.primitiveIndexResource = device->CreateGpuBuffer(desc);
		}

		// CullData resource
		{
			GpuBufferDesc desc{
				.name			= format("Mesh {} - CullData Resource", i),
				.resourceType	= ResourceType::StructuredBuffer,
				.memoryAccess	= MemoryAccess::GpuRead,
				.elementCount	= m.cullingData.size(),
				.elementSize	= sizeof(CullData),
				.initialData	= m.cullingData.data()
			};
			m.cullDataResource = device->CreateGpuBuffer(desc);
		}

		// MeshInfo resource
		{
			MeshInfo info{
				.indexSize				= m.indexSize,
				.meshletCount			= (uint32_t)m.meshlets.size(),
				.lastMeshletVertCount	= m.meshlets.back().vertCount,
				.lastMeshletPrimCount	= m.meshlets.back().primCount
			};

			GpuBufferDesc desc{
				.name			= format("Mesh {} - MeshInfo Resource", i),
				.resourceType	= ResourceType::ConstantBuffer,
				.memoryAccess	= MemoryAccess::CpuWrite | MemoryAccess::GpuRead,
				.elementCount	= 1,
				.elementSize	= sizeof(MeshInfo),
				.initialData	= &info
			};
			m.meshInfoResource = device->CreateGpuBuffer(desc);
		}
	}

	return S_OK;
}