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
#include "Graphics\Texture.h"

// TODO: Going to need a ModelManager (like TextureManager) to cache models loaded from file.

namespace Luna
{

// Forward declarations
class GraphicsContext;
class IDevice;
class VertexLayoutBase;


enum class ModelLoad
{
	CalcTangentSpace			= 1 << 0,
	JoinIdenticalVertices		= 1 << 1,
	MakeLeftHanded				= 1 << 2,
	Triangulate					= 1 << 3,
	RemoveComponent				= 1 << 4,
	GenNormals					= 1 << 5,
	GenSmoothNormals			= 1 << 6,
	SplitLargeMeshes			= 1 << 7,
	PreTransformVertices		= 1 << 8,
	LimitBoneWeights			= 1 << 9,
	ValidateDataStructure		= 1 << 10,
	ImproveCacheLocality		= 1 << 11,
	RemoveRedundantMaterials	= 1 << 12,
	FixInfacingNormals			= 1 << 13,
	SortByPType					= 1 << 14,
	FindDegenerates				= 1 << 15,
	FindInvalidData				= 1 << 16,
	GenUVCoords					= 1 << 17,
	TransformUVCoords			= 1 << 18,
	FindInstances				= 1 << 19,
	OptimizeMeshes				= 1 << 20,
	OptimizeGraph				= 1 << 21,
	FlipUVs						= 1 << 22,
	FlipWindingOrder			= 1 << 23,
	SplitByBoneCount			= 1 << 24,
	Debone						= 1 << 25,

	ConvertToLeftHandded = MakeLeftHanded |
	FlipUVs |
	FlipWindingOrder,

	TargetRealtime_Fast = CalcTangentSpace |
	GenNormals |
	JoinIdenticalVertices |
	Triangulate |
	GenUVCoords |
	SortByPType,

	TargetRealtime_Quality = CalcTangentSpace |
	GenSmoothNormals |
	JoinIdenticalVertices |
	Triangulate |
	GenUVCoords |
	SortByPType |
	ImproveCacheLocality |
	LimitBoneWeights |
	RemoveRedundantMaterials |
	SplitLargeMeshes |
	FindDegenerates |
	FindInvalidData,

	TargetRealtime_MaxQuality = TargetRealtime_Quality |
	FindInstances |
	ValidateDataStructure |
	OptimizeMeshes,

	StandardDefault = FlipUVs |
	Triangulate |
	PreTransformVertices |
	CalcTangentSpace
};

template <> struct EnableBitmaskOperators<ModelLoad> { static const bool enable = true; };

struct MeshMaterial
{ 
	Color diffuseColor{ DirectX::Colors::Black };
	TexturePtr diffuseTexture;
};


struct MeshPart
{
	uint32_t vertexBase{ 0 };
	uint32_t vertexCount{ 0 };
	uint32_t indexBase{ 0 };
	uint32_t indexCount{ 0 };
};


struct Mesh
{
	void Render(GraphicsContext& context, bool positionOnly = false);

	std::string name;

	int materialIndex{ -1 };

	GpuBufferPtr vertexBuffer;
	GpuBufferPtr vertexBufferPositionOnly;
	GpuBufferPtr indexBuffer;

	Math::Matrix4 meshToModelMatrix{ Math::kIdentity };
	Math::BoundingBox boundingBox;

	std::vector<MeshPart> meshParts;

	struct Model* model{ nullptr };
};

using MeshPtr = std::shared_ptr<Mesh>;


struct Model
{
	void Render(GraphicsContext& context, bool positionOnly = false);

	std::string name;

	Math::Matrix4 matrix{ Math::kIdentity };
	Math::Matrix4 prevMatrix{ Math::kIdentity };
	Math::BoundingBox boundingBox;

	std::vector<MeshPtr> meshes;
	std::vector<MeshMaterial> materials;
};

using ModelPtr = std::shared_ptr<Model>;


ModelPtr LoadModel(IDevice* device, const std::string& filename, const VertexLayoutBase& layout, float scale = 1.0f, ModelLoad loadFlags = ModelLoad::StandardDefault, bool loadMaterials = false);

ModelPtr MakePlane(IDevice* device, const VertexLayoutBase& layout, float width, float height);
ModelPtr MakeCylinder(IDevice* device, const VertexLayoutBase& layout, float height, float radius, uint32_t numVerts);
ModelPtr MakeSphere(IDevice* device, const VertexLayoutBase& layout, float radius, uint32_t numVerts, uint32_t numRings);
ModelPtr MakeBox(IDevice* device, const VertexLayoutBase& layout, float width, float height, float depth);

inline LogCategory LogModel{ "LogModel" };

} // namespace Luna