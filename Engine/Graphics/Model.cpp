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

#include "Model.h"

#include "Filesystem.h"
#include "Graphics\CommandContext.h"
#include "Graphics\Device.h"
#include "Graphics\InputLayout.h"

#include <assimp/Importer.hpp> 
#include <assimp/scene.h>     
#include <assimp/postprocess.h>
#include <assimp/cimport.h>


using namespace Math;
using namespace std;


namespace
{

void UpdateExtents(Math::Vector3& minExtents, Math::Vector3& maxExtents, const Math::Vector3& pos)
{
	minExtents = Math::Min(minExtents, pos);
	maxExtents = Math::Max(maxExtents, pos);
}

uint32_t GetPreprocessFlags(Luna::ModelLoad modelLoadFlags)
{
	uint32_t flags = 0;

	flags |= HasFlag(modelLoadFlags, Luna::ModelLoad::CalcTangentSpace) ? aiProcess_CalcTangentSpace : 0;
	flags |= HasFlag(modelLoadFlags, Luna::ModelLoad::JoinIdenticalVertices) ? aiProcess_JoinIdenticalVertices : 0;
	flags |= HasFlag(modelLoadFlags, Luna::ModelLoad::MakeLeftHanded) ? aiProcess_CalcTangentSpace : 0;
	flags |= HasFlag(modelLoadFlags, Luna::ModelLoad::Triangulate) ? aiProcess_Triangulate : 0;
	flags |= HasFlag(modelLoadFlags, Luna::ModelLoad::RemoveComponent) ? aiProcess_RemoveComponent : 0;
	flags |= HasFlag(modelLoadFlags, Luna::ModelLoad::GenNormals) ? aiProcess_GenNormals : 0;
	flags |= HasFlag(modelLoadFlags, Luna::ModelLoad::GenSmoothNormals) ? aiProcess_GenSmoothNormals : 0;
	flags |= HasFlag(modelLoadFlags, Luna::ModelLoad::SplitLargeMeshes) ? aiProcess_SplitLargeMeshes : 0;
	flags |= HasFlag(modelLoadFlags, Luna::ModelLoad::PreTransformVertices) ? aiProcess_PreTransformVertices : 0;
	flags |= HasFlag(modelLoadFlags, Luna::ModelLoad::LimitBoneWeights) ? aiProcess_LimitBoneWeights : 0;
	flags |= HasFlag(modelLoadFlags, Luna::ModelLoad::ValidateDataStructure) ? aiProcess_ValidateDataStructure : 0;
	flags |= HasFlag(modelLoadFlags, Luna::ModelLoad::ImproveCacheLocality) ? aiProcess_ImproveCacheLocality : 0;
	flags |= HasFlag(modelLoadFlags, Luna::ModelLoad::RemoveRedundantMaterials) ? aiProcess_RemoveRedundantMaterials : 0;
	flags |= HasFlag(modelLoadFlags, Luna::ModelLoad::FixInfacingNormals) ? aiProcess_FixInfacingNormals : 0;
	flags |= HasFlag(modelLoadFlags, Luna::ModelLoad::SortByPType) ? aiProcess_SortByPType : 0;
	flags |= HasFlag(modelLoadFlags, Luna::ModelLoad::FindDegenerates) ? aiProcess_FindDegenerates : 0;
	flags |= HasFlag(modelLoadFlags, Luna::ModelLoad::FindInvalidData) ? aiProcess_FindInvalidData : 0;
	flags |= HasFlag(modelLoadFlags, Luna::ModelLoad::GenUVCoords) ? aiProcess_GenUVCoords : 0;
	flags |= HasFlag(modelLoadFlags, Luna::ModelLoad::TransformUVCoords) ? aiProcess_TransformUVCoords : 0;
	flags |= HasFlag(modelLoadFlags, Luna::ModelLoad::FindInstances) ? aiProcess_FindInstances : 0;
	flags |= HasFlag(modelLoadFlags, Luna::ModelLoad::OptimizeMeshes) ? aiProcess_OptimizeMeshes : 0;
	flags |= HasFlag(modelLoadFlags, Luna::ModelLoad::OptimizeGraph) ? aiProcess_OptimizeGraph : 0;
	flags |= HasFlag(modelLoadFlags, Luna::ModelLoad::FlipUVs) ? aiProcess_FlipUVs : 0;
	flags |= HasFlag(modelLoadFlags, Luna::ModelLoad::FlipWindingOrder) ? aiProcess_FlipWindingOrder : 0;
	flags |= HasFlag(modelLoadFlags, Luna::ModelLoad::SplitByBoneCount) ? aiProcess_SplitByBoneCount : 0;
	flags |= HasFlag(modelLoadFlags, Luna::ModelLoad::Debone) ? aiProcess_Debone : 0;

	return flags;
}

} // anonymous namespace


namespace Luna
{

void Mesh::Render(GraphicsContext& context, bool positionOnly)
{
	context.SetIndexBuffer(indexBuffer);
	context.SetVertexBuffer(0, positionOnly ? vertexBufferPositionOnly : vertexBuffer);

	for (const auto& meshPart : meshParts)
	{
		context.DrawIndexed(meshPart.indexCount, meshPart.indexBase, meshPart.vertexBase);
	}
}


void Model::Render(GraphicsContext& context, bool positionOnly)
{
	for (auto mesh : meshes)
	{
		mesh->Render(context, positionOnly);
	}
}


ModelPtr LoadModel(IDevice* device, const string& filename, const VertexLayoutBase& layout, float scale, ModelLoad modelLoadFlags)
{
	const string fullpath = GetFileSystem()->GetFullPath(filename);
	assert(!fullpath.empty());

	Assimp::Importer aiImporter;

	const auto aiScene = aiImporter.ReadFile(fullpath.c_str(), GetPreprocessFlags(modelLoadFlags));
	if (!aiScene)
	{
		LogFatal << "Failed to load model file " << filename << endl;
		string errorStr = aiImporter.GetErrorString();
		LogFatal << errorStr << endl;
		return nullptr;
	}

	ModelPtr model = make_shared<Model>();

	model->meshes.reserve(aiScene->mNumMeshes);

	uint32_t vertexCount = 0;
	uint32_t indexCount = 0;

	const aiVector3D zero(0.0f, 0.0f, 0.0f);

	vector<float> vertexData;
	vector<float> vertexDataPositionOnly;
	vector<uint32_t> indexData;

	const VertexComponent components = layout.GetComponents();

	// Min/max for bounding box computation
	constexpr float maxF = std::numeric_limits<float>::max();
	Vector3 minExtents{ maxF, maxF, maxF };
	Vector3 maxExtents{ -maxF, -maxF, -maxF };

	for (uint32_t i = 0; i < aiScene->mNumMeshes; ++i)
	{
		const auto aiMesh = aiScene->mMeshes[i];

		MeshPtr mesh = make_shared<Mesh>();
		MeshPart meshPart{};

		meshPart.vertexBase = vertexCount;

		vertexCount += aiMesh->mNumVertices;

		aiColor3D color(0.0f, 0.0f, 0.0f);
		aiScene->mMaterials[aiMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, color);

		for (uint32_t j = 0; j < aiMesh->mNumVertices; ++j)
		{
			const aiVector3D* pos = &(aiMesh->mVertices[j]);
			const aiVector3D* normal = &(aiMesh->mNormals[j]);
			const aiVector3D* texCoord = (aiMesh->HasTextureCoords(0)) ? &(aiMesh->mTextureCoords[0][j]) : &zero;
			const aiVector3D* tangent = (aiMesh->HasTangentsAndBitangents()) ? &(aiMesh->mTangents[j]) : &zero;
			const aiVector3D* bitangent = (aiMesh->HasTangentsAndBitangents()) ? &(aiMesh->mBitangents[j]) : &zero;

			if (HasFlag(components, VertexComponent::Position))
			{
				vertexData.push_back(pos->x * scale);
				vertexData.push_back(pos->y * scale);  // TODO: Is this a hack?
				vertexData.push_back(pos->z * scale);

				vertexDataPositionOnly.push_back(pos->x * scale);
				vertexDataPositionOnly.push_back(pos->y * scale);  // TODO: Is this a hack?
				vertexDataPositionOnly.push_back(pos->z * scale);

				UpdateExtents(minExtents, maxExtents, scale * Math::Vector3(pos->x, pos->y, pos->z));
			}

			if (HasFlag(components, VertexComponent::Normal))
			{
				vertexData.push_back(normal->x);
				vertexData.push_back(normal->y); // TODO: Is this a hack?
				vertexData.push_back(normal->z);
			}

			if (HasFlag(components, VertexComponent::Tangent))
			{
				vertexData.push_back(tangent->x);
				vertexData.push_back(tangent->y);
				vertexData.push_back(tangent->z);
			}

			if (HasFlag(components, VertexComponent::Bitangent))
			{
				vertexData.push_back(bitangent->x);
				vertexData.push_back(bitangent->y);
				vertexData.push_back(bitangent->z);
			}

			if (HasFlag(components, VertexComponent::Color))
			{
				vertexData.push_back(color.r);
				vertexData.push_back(color.g);
				vertexData.push_back(color.b);
				vertexData.push_back(1.0f);
			}

			// TODO Color1

			if (HasFlag(components, VertexComponent::Texcoord))
			{
				vertexData.push_back(texCoord->x);
				vertexData.push_back(texCoord->y);
			}

			// TODO Texcoord1-3, BlendIndices, BlendWeight
		}

		meshPart.vertexCount = aiMesh->mNumVertices;

		uint32_t indexBase = static_cast<uint32_t>(indexData.size());
		for (unsigned int j = 0; j < aiMesh->mNumFaces; j++)
		{
			const aiFace& Face = aiMesh->mFaces[j];
			if (Face.mNumIndices != 3)
				continue;
			indexData.push_back(indexBase + Face.mIndices[0]);
			indexData.push_back(indexBase + Face.mIndices[1]);
			indexData.push_back(indexBase + Face.mIndices[2]);
			meshPart.indexCount += 3;
			indexCount += 3;
		}

		// Create vertex buffer
		uint32_t stride = layout.GetSizeInBytes();

		GpuBufferDesc vertexBufferDesc{
			.name			= "Model|VertexBuffer",
			.resourceType	= ResourceType::VertexBuffer,
			.memoryAccess	= MemoryAccess::GpuRead,
			.elementCount	= sizeof(float) * vertexData.size() / stride,
			.elementSize	= stride,
			.initialData	= vertexData.data()
		};
		mesh->vertexBuffer = device->CreateGpuBuffer(vertexBufferDesc);

		// Create position-only vertex buffer
		stride = 3 * sizeof(float);
		GpuBufferDesc positionOnlyVertexBufferDesc
		{
			.name			= "Model|VertexBuffer (Position Only)",
			.resourceType	= ResourceType::VertexBuffer,
			.memoryAccess	= MemoryAccess::GpuRead,
			.elementCount	= sizeof(float) * vertexDataPositionOnly.size() / stride,
			.elementSize	= stride,
			.initialData	= vertexDataPositionOnly.data()
		};
		mesh->vertexBufferPositionOnly = device->CreateGpuBuffer(positionOnlyVertexBufferDesc);
		
		// TODO: Support uint16 indices

		// Create index buffer
		GpuBufferDesc indexBufferDesc{
			.name			= "Model|IndexBuffer",
			.resourceType	= ResourceType::IndexBuffer,
			.memoryAccess	= MemoryAccess::GpuRead,
			.elementCount	= indexData.size(),
			.elementSize	= sizeof(uint32_t),
			.initialData	= indexData.data()
		};
		mesh->indexBuffer = device->CreateGpuBuffer(indexBufferDesc);
		
		// Set bounding box
		mesh->boundingBox = Math::BoundingBoxFromMinMax(minExtents, maxExtents);

		mesh->meshParts.push_back(meshPart);
		model->meshes.push_back(mesh);
	}

	return model;
}


ModelPtr MakePlane(IDevice* device, const VertexLayoutBase& layout, float width, float height)
{
	bool bHasNormals = HasFlag(layout.GetComponents(), VertexComponent::Normal);
	bool bHasUVs = HasFlag(layout.GetComponents(), VertexComponent::Texcoord);

	uint32_t stride = 3 * sizeof(float);
	stride += bHasNormals ? (3 * sizeof(float)) : 0;
	stride += bHasUVs ? (2 * sizeof(float)) : 0;

	vector<float> vertices;
	vector<float> verticesPositionOnly;
	size_t vertexSize = 3; // position
	vertexSize += bHasNormals ? 3 : 0;
	vertexSize += bHasUVs ? 2 : 0;
	vertices.reserve(4 * vertexSize);
	verticesPositionOnly.reserve(4 * 3);

	// Vertex 0
	vertices.push_back(width / 2.0f);
	vertices.push_back(0.0f);
	vertices.push_back(-height / 2.0f);
	if (bHasNormals)
	{
		vertices.push_back(0.0f);
		vertices.push_back(1.0f);
		vertices.push_back(0.0f);
	}
	if (bHasUVs)
	{
		vertices.push_back(0.0f);
		vertices.push_back(0.0f);
	}
	verticesPositionOnly.push_back(width / 2.0f);
	verticesPositionOnly.push_back(0.0f);
	verticesPositionOnly.push_back(-height / 2.0f);

	// Vertex 1
	vertices.push_back(width / 2.0f);
	vertices.push_back(0.0f);
	vertices.push_back(height / 2.0f);
	if (bHasNormals)
	{
		vertices.push_back(0.0f);
		vertices.push_back(1.0f);
		vertices.push_back(0.0f);
	}
	if (bHasUVs)
	{
		vertices.push_back(1.0f);
		vertices.push_back(0.0f);
	}
	verticesPositionOnly.push_back(width / 2.0f);
	verticesPositionOnly.push_back(0.0f);
	verticesPositionOnly.push_back(height / 2.0f);

	// Vertex 2
	vertices.push_back(-width / 2.0f);
	vertices.push_back(0.0f);
	vertices.push_back(-height / 2.0f);
	if (bHasNormals)
	{
		vertices.push_back(0.0f);
		vertices.push_back(1.0f);
		vertices.push_back(0.0f);
	}
	if (bHasUVs)
	{
		vertices.push_back(0.0f);
		vertices.push_back(1.0f);
	}
	verticesPositionOnly.push_back(-width / 2.0f);
	verticesPositionOnly.push_back(0.0f);
	verticesPositionOnly.push_back(-height / 2.0f);

	// Vertex 3
	vertices.push_back(-width / 2.0f);
	vertices.push_back(0.0f);
	vertices.push_back(height / 2.0f);
	if (bHasNormals)
	{
		vertices.push_back(0.0f);
		vertices.push_back(1.0f);
		vertices.push_back(0.0f);
	}
	if (bHasUVs)
	{
		vertices.push_back(1.0f);
		vertices.push_back(1.0f);
	}
	verticesPositionOnly.push_back(-width / 2.0f);
	verticesPositionOnly.push_back(0.0f);
	verticesPositionOnly.push_back(height / 2.0f);

	auto model = make_shared<Model>();
	auto mesh = make_shared<Mesh>();

	// Create vertex buffer
	GpuBufferDesc vertexBufferDesc{
		.name			= "Plane|VertexBuffer",
		.resourceType	= ResourceType::VertexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= vertices.size(),
		.elementSize	= stride,
		.initialData	= vertices.data()
	};
	mesh->vertexBuffer = device->CreateGpuBuffer(vertexBufferDesc);

	// Create position-only vertex buffer
	stride = 3 * sizeof(float);
	GpuBufferDesc positionOnlyVertexBufferDesc{
		.name			= "Plane|VertexBuffer (Position Only)",
		.resourceType	= ResourceType::VertexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= verticesPositionOnly.size(),
		.elementSize	= stride,
		.initialData	= verticesPositionOnly.data()
	};
	mesh->vertexBufferPositionOnly = device->CreateGpuBuffer(positionOnlyVertexBufferDesc);
	
	// Create index buffer
	vector<uint16_t> indices{ 0, 2, 1, 3, 1, 2 };
	GpuBufferDesc indexBufferDesc{
		.name			= "Plane|IndexBuffer",
		.resourceType	= ResourceType::IndexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= indices.size(),
		.elementSize	= sizeof(uint16_t),
		.initialData	= indices.data()
	};
	mesh->indexBuffer = device->CreateGpuBuffer(indexBufferDesc);

	// Set bounding box
	mesh->boundingBox = BoundingBox(Vector3(Math::kZero), Vector3(width / 2.0f, 0.0, height / 2.0f));
	model->boundingBox = mesh->boundingBox;

	MeshPart meshPart{ .indexCount = (uint32_t)indices.size() };

	mesh->meshParts.push_back(meshPart);
	model->meshes.push_back(mesh);

	return model;
}


ModelPtr MakeCylinder(IDevice* device, const VertexLayoutBase& layout, float height, float radius, uint32_t numVerts)
{
	bool bHasNormals = HasFlag(layout.GetComponents(), VertexComponent::Normal);
	bool bHasUVs = HasFlag(layout.GetComponents(), VertexComponent::Texcoord);

	uint32_t stride = 3 * sizeof(float);
	stride += bHasNormals ? (3 * sizeof(float)) : 0;
	stride += bHasUVs ? (2 * sizeof(float)) : 0;

	const size_t totalVerts = 4 * numVerts + 2;

	vector<float> vertices;
	vector<float> verticesPositionOnly;
	size_t vertexSize = 3; // position
	vertexSize += bHasNormals ? 3 : 0;
	vertexSize += bHasUVs ? 2 : 0;
	vertices.reserve(totalVerts * vertexSize);
	verticesPositionOnly.reserve(totalVerts * 3);

	vector<uint16_t> indices;

	float deltaPhi = DirectX::XM_2PI / static_cast<float>(numVerts - 1);

	// Cylinder side
	float phi = 0.0f;
	for (uint32_t i = 0; i < numVerts; ++i)
	{
		const float nx = sinf(phi);
		const float nz = cosf(phi);
		const float x = radius * nx;
		const float z = radius * nz;
		const float u = phi / DirectX::XM_2PI;

		// Position top
		vertices.push_back(x);
		vertices.push_back(height);
		vertices.push_back(z);

		if (bHasNormals)
		{
			vertices.push_back(nx);
			vertices.push_back(0.0f);
			vertices.push_back(nz);
		}

		if (bHasUVs)
		{
			vertices.push_back(u);
			vertices.push_back(1.0);
		}

		verticesPositionOnly.push_back(x);
		verticesPositionOnly.push_back(height);
		verticesPositionOnly.push_back(z);

		// Position bottom
		vertices.push_back(x);
		vertices.push_back(0.0f);
		vertices.push_back(z);

		if (bHasNormals)
		{
			vertices.push_back(nx);
			vertices.push_back(0.0f);
			vertices.push_back(nz);
		}

		if (bHasUVs)
		{
			vertices.push_back(u);
			vertices.push_back(0.0);
		}

		verticesPositionOnly.push_back(x);
		verticesPositionOnly.push_back(0.0f);
		verticesPositionOnly.push_back(z);

		indices.push_back(2 * i);
		indices.push_back(2 * i + 1);

		phi += deltaPhi;
	}

	// Restart strip
	indices.push_back(0xFFFF);

	// Cylinder bottom
	phi = 0.0f;
	uint16_t startVert = 2 * numVerts;
	for (uint32_t i = 0; i < numVerts; ++i)
	{
		const float nx = sinf(phi);
		const float nz = cosf(phi);
		const float x = radius * nx;
		const float z = radius * nz;

		// Position bottom
		vertices.push_back(x);
		vertices.push_back(0.0f);
		vertices.push_back(z);

		if (bHasNormals)
		{
			vertices.push_back(0.0f);
			vertices.push_back(-1.0f);
			vertices.push_back(0.0f);
		}

		if (bHasUVs)
		{
			vertices.push_back(0.5f * nx + 0.5f);
			vertices.push_back(0.5f * nz + 0.5f);
		}

		verticesPositionOnly.push_back(x);
		verticesPositionOnly.push_back(0.0f);
		verticesPositionOnly.push_back(z);

		indices.push_back(3 * numVerts);
		indices.push_back(startVert + i);

		phi += deltaPhi;
	}
	// Position center
	vertices.push_back(0.0f);
	vertices.push_back(0.0f);
	vertices.push_back(0.0f);

	if (bHasNormals)
	{
		vertices.push_back(0.0f);
		vertices.push_back(-1.0f);
		vertices.push_back(0.0f);
	}

	if (bHasUVs)
	{
		vertices.push_back(0.5f);
		vertices.push_back(0.5f);
	}

	verticesPositionOnly.push_back(0.0f);
	verticesPositionOnly.push_back(0.0f);
	verticesPositionOnly.push_back(0.0f);

	// Restart strip
	indices.push_back(0xFFFF);

	// Cylinder top
	phi = 0.0f;
	startVert = 3 * numVerts + 1;
	for (uint32_t i = 0; i < numVerts; ++i)
	{
		const float nx = sinf(phi);
		const float nz = cosf(phi);
		const float x = radius * nx;
		const float z = radius * nz;

		// Position top
		vertices.push_back(x);
		vertices.push_back(height);
		vertices.push_back(z);

		if (bHasNormals)
		{
			vertices.push_back(0.0f);
			vertices.push_back(1.0f);
			vertices.push_back(0.0f);
		}

		if (bHasUVs)
		{
			vertices.push_back(0.5f * nx + 0.5f);
			vertices.push_back(0.5f * nz + 0.5f);
		}

		verticesPositionOnly.push_back(x);
		verticesPositionOnly.push_back(height);
		verticesPositionOnly.push_back(z);

		indices.push_back(4 * numVerts + 1);
		indices.push_back(startVert + i);

		phi += deltaPhi;
	}
	// Position center
	vertices.push_back(0.0f);
	vertices.push_back(height);
	vertices.push_back(0.0f);

	if (bHasNormals)
	{
		vertices.push_back(0.0f);
		vertices.push_back(1.0f);
		vertices.push_back(0.0f);
	}

	if (bHasUVs)
	{
		vertices.push_back(0.5f);
		vertices.push_back(0.5f);
	}

	verticesPositionOnly.push_back(0.0f);
	verticesPositionOnly.push_back(height);
	verticesPositionOnly.push_back(0.0f);

	auto model = make_shared<Model>();
	auto mesh = make_shared<Mesh>();

	assert(totalVerts == vertices.size() / vertexSize);
	
	// Create vertex buffer
	GpuBufferDesc vertexBufferDesc{
		.name			= "Cylinder|VertexBuffer",
		.resourceType	= ResourceType::VertexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= vertices.size(),
		.elementSize	= stride,
		.initialData	= vertices.data()
	};
	mesh->vertexBuffer = device->CreateGpuBuffer(vertexBufferDesc);

	// Create position-only vertex buffer
	stride = 3 * sizeof(float);
	GpuBufferDesc positionOnlyVertexBufferDesc{
		.name			= "Cylinder|VertexBuffer (Position Only)",
		.resourceType	= ResourceType::VertexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= verticesPositionOnly.size(),
		.elementSize	= stride,
		.initialData	= verticesPositionOnly.data()
	};
	mesh->vertexBufferPositionOnly = device->CreateGpuBuffer(positionOnlyVertexBufferDesc);

	// Create index buffer
	GpuBufferDesc indexBufferDesc{
		.name			= "Cylinder|IndexBuffer",
		.resourceType	= ResourceType::IndexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= indices.size(),
		.elementSize	= sizeof(uint16_t),
		.initialData	= indices.data()
	};
	mesh->indexBuffer = device->CreateGpuBuffer(indexBufferDesc);

	mesh->boundingBox = BoundingBoxFromMinMax(Vector3(-radius, 0.0f, -radius), Vector3(radius, height, radius));
	model->boundingBox = mesh->boundingBox;

	MeshPart meshPart{ .indexCount = (uint32_t)indices.size() };

	mesh->meshParts.push_back(meshPart);
	model->meshes.push_back(mesh);

	return model;
}


ModelPtr MakeSphere(IDevice* device, const VertexLayoutBase& layout, float radius, uint32_t numVerts, uint32_t numRings)
{
	bool bHasNormals = HasFlag(layout.GetComponents(), VertexComponent::Normal);
	bool bHasUVs = HasFlag(layout.GetComponents(), VertexComponent::Texcoord);

	uint32_t stride = 3 * sizeof(float);
	stride += bHasNormals ? (3 * sizeof(float)) : 0;
	stride += bHasUVs ? (2 * sizeof(float)) : 0;

	const size_t totalVerts = numVerts * numRings;

	vector<float> vertices;
	vector<float> verticesPositionOnly;
	size_t vertexSize = 3; // position
	vertexSize += bHasNormals ? 3 : 0;
	vertexSize += bHasUVs ? 2 : 0;
	vertices.reserve(totalVerts * vertexSize);
	verticesPositionOnly.reserve(totalVerts * 3);

	vector<uint16_t> indices;

	float phi = 0.0f;
	float theta = 0.0f;
	float deltaPhi = DirectX::XM_2PI / static_cast<float>(numVerts - 1);
	float deltaTheta = DirectX::XM_PI / static_cast<float>(numRings - 1);

	uint16_t curVert = 0;
	for (uint32_t i = 0; i < numRings; ++i)
	{
		phi = 0.0f;
		for (uint32_t j = 0; j < numVerts; ++j)
		{
			float nx = sinf(theta) * cosf(phi);
			float ny = cosf(theta);
			float nz = sinf(theta) * sinf(phi);

			vertices.push_back(radius * nx);
			vertices.push_back(radius * ny);
			vertices.push_back(radius * nz);

			if (bHasNormals)
			{
				vertices.push_back(nx);
				vertices.push_back(ny);
				vertices.push_back(nz);
			}

			if (bHasUVs)
			{
				vertices.push_back(float(j) / float(numVerts - 1));
				vertices.push_back(float(i) / float(numRings - 1));
			}

			verticesPositionOnly.push_back(radius * nx);
			verticesPositionOnly.push_back(radius * ny);
			verticesPositionOnly.push_back(radius * nz);

			indices.push_back(curVert + numVerts);
			indices.push_back(curVert);

			++curVert;
			phi += deltaPhi;
		}

		indices.push_back(0xFFFF);

		theta += deltaTheta;
	}

	auto model = make_shared<Model>();
	auto mesh = make_shared<Mesh>();

	assert(totalVerts == vertices.size() / vertexSize);
	
	// Create vertex buffer
	GpuBufferDesc vertexBufferDesc{
		.name			= "Sphere|VertexBuffer",
		.resourceType	= ResourceType::VertexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= vertices.size(),
		.elementSize	= stride,
		.initialData	= vertices.data()
	};
	mesh->vertexBuffer = device->CreateGpuBuffer(vertexBufferDesc);

	// Create position-only vertex buffer
	stride = 3 * sizeof(float);
	GpuBufferDesc positionOnlyVertexBufferDesc{
		.name			= "Sphere|VertexBuffer (Position Only)",
		.resourceType	= ResourceType::VertexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= verticesPositionOnly.size(),
		.elementSize	= stride,
		.initialData	= verticesPositionOnly.data()
	};
	mesh->vertexBufferPositionOnly = device->CreateGpuBuffer(positionOnlyVertexBufferDesc);

	// Create index buffer
	GpuBufferDesc indexBufferDesc{
		.name			= "Sphere|IndexBuffer",
		.resourceType	= ResourceType::IndexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= indices.size(),
		.elementSize	= sizeof(uint16_t),
		.initialData	= indices.data()
	};
	mesh->indexBuffer = device->CreateGpuBuffer(indexBufferDesc);

	// Set bounding box
	mesh->boundingBox = BoundingBoxFromMinMax(Vector3(-radius, -radius, -radius), Vector3(radius, radius, radius));
	model->boundingBox = mesh->boundingBox;

	MeshPart meshPart{ .indexCount = (uint32_t)indices.size() };

	mesh->meshParts.push_back(meshPart);
	model->meshes.push_back(mesh);

	return model;
}


ModelPtr MakeBox(IDevice* device, const VertexLayoutBase& layout, float width, float height, float depth)
{
	bool bHasNormals = HasFlag(layout.GetComponents(), VertexComponent::Normal);
	bool bHasUVs = HasFlag(layout.GetComponents(), VertexComponent::Texcoord);

	uint32_t stride = 3 * sizeof(float);
	stride += bHasNormals ? (3 * sizeof(float)) : 0;
	stride += bHasUVs ? (2 * sizeof(float)) : 0;

	const size_t totalVerts = 24;

	vector<float> vertices;
	vector<float> verticesPositionOnly;
	size_t vertexSize = 3; // position
	vertexSize += bHasNormals ? 3 : 0;
	vertexSize += bHasUVs ? 2 : 0;
	vertices.reserve(totalVerts * vertexSize);
	verticesPositionOnly.reserve(totalVerts * 3);

	const float hwidth = 0.5f * width;
	const float hheight = 0.5f * height;
	const float hdepth = 0.5f * depth;

	auto InsertVertex = [&vertices, &verticesPositionOnly, bHasNormals, bHasUVs](float x, float y, float z, float nx, float ny, float nz, float u, float v)
		{
			// Position
			vertices.push_back(x);
			vertices.push_back(y);
			vertices.push_back(z);

			// Normal
			if (bHasNormals)
			{
				vertices.push_back(nx);
				vertices.push_back(ny);
				vertices.push_back(nz);
			}

			// UV
			if (bHasUVs)
			{
				vertices.push_back(u);
				vertices.push_back(v);
			}

			// Position only
			verticesPositionOnly.push_back(x);
			verticesPositionOnly.push_back(y);
			verticesPositionOnly.push_back(z);
		};

	// -X face
	InsertVertex(-hwidth, -hheight, -hdepth, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	InsertVertex(-hwidth, hheight, -hdepth, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	InsertVertex(-hwidth, -hheight, hdepth, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	InsertVertex(-hwidth, hheight, hdepth, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	// +X face
	InsertVertex(hwidth, -hheight, hdepth, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	InsertVertex(hwidth, hheight, hdepth, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	InsertVertex(hwidth, -hheight, -hdepth, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	InsertVertex(hwidth, hheight, -hdepth, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	// -Y face
	InsertVertex(-hwidth, -hheight, -hdepth, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f);
	InsertVertex(-hwidth, -hheight, hdepth, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f);
	InsertVertex(hwidth, -hheight, -hdepth, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f);
	InsertVertex(hwidth, -hheight, hdepth, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f);
	// +Y face
	InsertVertex(hwidth, hheight, -hdepth, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
	InsertVertex(hwidth, hheight, hdepth, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f);
	InsertVertex(-hwidth, hheight, -hdepth, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	InsertVertex(-hwidth, hheight, hdepth, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
	// -Z face
	InsertVertex(hwidth, -hheight, -hdepth, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	InsertVertex(hwidth, hheight, -hdepth, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);
	InsertVertex(-hwidth, -hheight, -hdepth, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	InsertVertex(-hwidth, hheight, -hdepth, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	// +Z face
	InsertVertex(-hwidth, -hheight, hdepth, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	InsertVertex(-hwidth, hheight, hdepth, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	InsertVertex(hwidth, -hheight, hdepth, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
	InsertVertex(hwidth, hheight, hdepth, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

	vector<uint16_t> indices = {
		1, 0, 3, 2, 0xFFFF,
		5, 4, 7, 6, 0xFFFF,
		9, 8, 11, 10, 0xFFFF,
		13, 12, 15, 14, 0xFFFF,
		17, 16, 19, 18, 0xFFFF,
		21, 20, 23, 22
	};

	auto model = make_shared<Model>();
	auto mesh = make_shared<Mesh>();

	assert(totalVerts == vertices.size() / vertexSize);
	
	// Create vertex buffer
	GpuBufferDesc vertexBufferDesc{
		.name			= "Box|VertexBuffer",
		.resourceType	= ResourceType::VertexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= vertices.size(),
		.elementSize	= stride,
		.initialData	= vertices.data()
	};
	mesh->vertexBuffer = device->CreateGpuBuffer(vertexBufferDesc);

	// Create position-only vertex buffer
	stride = 3 * sizeof(float);
	GpuBufferDesc positionOnlyVertexBufferDesc{
		.name			= "Box|VertexBuffer (Position Only)",
		.resourceType	= ResourceType::VertexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= verticesPositionOnly.size(),
		.elementSize	= stride,
		.initialData	= verticesPositionOnly.data()
	};
	mesh->vertexBufferPositionOnly = device->CreateGpuBuffer(positionOnlyVertexBufferDesc);

	// Create index buffer
	GpuBufferDesc indexBufferDesc{
		.name			= "Box|IndexBuffer",
		.resourceType	= ResourceType::IndexBuffer,
		.memoryAccess	= MemoryAccess::GpuRead,
		.elementCount	= indices.size(),
		.elementSize	= sizeof(uint16_t),
		.initialData	= indices.data()
	};
	mesh->indexBuffer = device->CreateGpuBuffer(indexBufferDesc);

	// Set bounding box
	mesh->boundingBox = BoundingBoxFromMinMax(Vector3(-hwidth, -hheight, -hdepth), Vector3(hwidth, hheight, hdepth));
	model->boundingBox = mesh->boundingBox;

	MeshPart meshPart{ .indexCount = (uint32_t)indices.size() };

	mesh->meshParts.push_back(meshPart);
	model->meshes.push_back(mesh);

	return model;
}

} // namespace Luna