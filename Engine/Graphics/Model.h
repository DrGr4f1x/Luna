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

// TODO: Going to need a ModelManager (like TextureManager) so we don't load the same model twice.

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


struct MeshPart
{
	uint32_t vertexBase{ 0 };
	uint32_t vertexCount{ 0 };
	uint32_t indexBase{ 0 };
	uint32_t indexCount{ 0 };
};


class Mesh
{
	friend class Model;

public:
	// Accessors
	void SetName(const std::string& name);
	const std::string& GetName() const { return m_name; }

	void AddMeshPart(MeshPart meshPart);
	size_t GetNumMeshParts() const { return m_meshParts.size(); }
	MeshPart& GetMeshPart(size_t index) { return m_meshParts[index]; }
	const MeshPart& GetMeshPart(size_t index) const { return m_meshParts[index]; }

	GpuBufferPtr GetVertexBuffer() { return m_vertexBuffer; }
	const GpuBufferPtr GetVertexBuffer() const { return m_vertexBuffer; }
	GpuBufferPtr GetIndexBuffer() { return m_indexBuffer; }
	const GpuBufferPtr GetIndexBuffer() const { return m_indexBuffer; }

	GpuBufferPtr GetPositionOnlyVertexBuffer() { return m_vertexBufferPositionOnly; }
	const GpuBufferPtr GetPositionOnlyVertexBuffer() const { return m_vertexBufferPositionOnly; }

	void SetMatrix(const Math::Matrix4& matrix);
	const Math::Matrix4 GetMatrix() const { return m_matrix; }

	void Render(GraphicsContext& context);
	void RenderPositionOnly(GraphicsContext& context);

private:
	std::string m_name;

	GpuBufferPtr m_vertexBuffer;
	GpuBufferPtr m_vertexBufferPositionOnly;
	GpuBufferPtr m_indexBuffer;

	Math::Matrix4 m_matrix{ Math::kIdentity };
	Math::BoundingBox m_boundingBox;

	std::vector<MeshPart> m_meshParts;

	class Model* m_model{ nullptr };
};

using MeshPtr = std::shared_ptr<Mesh>;


class Model
{
public:
	// Accessors
	void SetName(const std::string& name);
	const std::string& GetName() const { return m_name; }

	void AddMesh(MeshPtr mesh);
	size_t GetNumMeshes() const { return m_meshes.size(); }
	MeshPtr GetMesh(size_t index) const { return m_meshes[index]; }

	void SetMatrix(const Math::Matrix4& matrix);
	const Math::Matrix4 GetMatrix() const { return m_matrix; }
	void StorePrevMatrix();
	const Math::Matrix4 GetPrevMatrix() const { return m_prevMatrix; }

	const Math::BoundingBox& GetBoundingBox() const { return m_boundingBox; }

	void Render(GraphicsContext& context);
	void RenderPositionOnly(GraphicsContext& context);

	static std::shared_ptr<Model> Load(IDevice* device, const std::string& filename, const VertexLayoutBase& layout, float scale = 1.0f, ModelLoad loadFlags = ModelLoad::StandardDefault);

	static std::shared_ptr<Model> MakePlane(IDevice* device, const VertexLayoutBase& layout, float width, float height);
	static std::shared_ptr<Model> MakeCylinder(IDevice* device, const VertexLayoutBase& layout, float height, float radius, uint32_t numVerts);
	static std::shared_ptr<Model> MakeSphere(IDevice* device, const VertexLayoutBase& layout, float radius, uint32_t numVerts, uint32_t numRings);
	static std::shared_ptr<Model> MakeBox(IDevice* device, const VertexLayoutBase& layout, float width, float height, float depth);

protected:
	std::string m_name;

	Math::Matrix4 m_matrix{ Math::kIdentity };
	Math::Matrix4 m_prevMatrix{ Math::kIdentity };
	Math::BoundingBox m_boundingBox;

	std::vector<MeshPtr> m_meshes;
};

using ModelPtr = std::shared_ptr<Model>;

} // namespace Luna