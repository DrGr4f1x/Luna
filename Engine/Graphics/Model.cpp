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
#include "Graphics\Loaders\DDSTextureLoader.h"
#include "Graphics\Loaders\KTXTextureLoader.h"
#include "Graphics\Loaders\STBTextureLoader.h"

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

struct MeshMaterialData
{
	// Basic colors
	std::optional<Color> diffuse;
	std::optional<Color> specular;
	std::optional<Color> ambient;
	std::optional<Color> emissive;
	std::optional<Color> transparent;
	std::optional<Color> reflective;
	// Basic parameters
	std::optional<float> opacity;
	std::optional<float> shininess;
	std::optional<float> specularStrength;
	std::optional<float> transparencyFactor;
	std::optional<float> bumpScaling;
	std::optional<float> reflectivity;
	// Rendering options
	std::optional<int> twoSided;
	std::optional<int> shadingModel;
	std::optional<int> wireframe;
	std::optional<int> blendFunc;
	// Textures
	std::optional<std::string> diffuseTex;
	std::optional<std::string> specularTex;
	std::optional<std::string> ambientTex;
	std::optional<std::string> emissiveTex;
	std::optional<std::string> normalTex;
	std::optional<std::string> heightTex;
	std::optional<std::string> shininessTex;
	std::optional<std::string> opacityTex;
	std::optional<std::string> displacementTex;
	std::optional<std::string> lightmapTex;
	std::optional<std::string> reflectionTex;
};


void LogMaterialData(const MeshMaterialData& materialData)
{
	LogInfo(LogModel) << "Material parameters" << endl;

	auto LogColor = [](const optional<Color>& colorOpt, const string& name)
		{
			if (colorOpt.has_value())
			{
				Color c = colorOpt.value();
				LogInfo(LogModel) << format("  {}: ({}, {}, {}, {})", name, c.R(), c.G(), c.B(), c.A()) << endl;
			}
		};

	auto LogFloat = [](const optional<float>& floatOpt, const string& name)
		{
			if (floatOpt.has_value())
			{
				float f = floatOpt.value();
				LogInfo(LogModel) << format("  {}: {}", name, f) << endl;
			}
		};

	auto LogInt = [](const optional<int>& intOpt, const string& name)
		{
			if (intOpt.has_value())
			{
				int i = intOpt.value();
				LogInfo(LogModel) << format("  {}: {}", name, i) << endl;
			}
		};

	auto LogTexture = [](const optional<string>& strOpt, const string& name)
		{
			if (strOpt.has_value())
			{
				string str = strOpt.value();
				LogInfo(LogModel) << format("  {}: {}", name, str) << endl;
			}
		};

	LogColor(materialData.diffuse, "Diffuse");
	LogColor(materialData.specular, "Specular");
	LogColor(materialData.ambient, "Ambient");
	LogColor(materialData.emissive, "Emissive");
	LogColor(materialData.transparent, "Transparent");
	LogColor(materialData.reflective, "Reflective");
	LogFloat(materialData.opacity, "Opacity");
	LogFloat(materialData.shininess, "Shininess");
	LogFloat(materialData.specularStrength, "Specular strength");
	LogFloat(materialData.transparencyFactor, "Transparency factor");
	LogFloat(materialData.bumpScaling, "Bump scaling");
	LogFloat(materialData.reflectivity, "Reflectivity");
	LogInt(materialData.twoSided, "Two-sided");
	LogInt(materialData.shadingModel, "Shading model");
	LogInt(materialData.wireframe, "Wireframe");
	LogInt(materialData.blendFunc, "Blend func");
	LogTexture(materialData.diffuseTex, "Diffuse tex");
	LogTexture(materialData.specularTex, "Specular tex");
	LogTexture(materialData.ambientTex, "Ambient tex");
	LogTexture(materialData.emissiveTex, "Emissive tex");
	LogTexture(materialData.normalTex, "Normal tex");
	LogTexture(materialData.heightTex, "Height tex");
	LogTexture(materialData.shininessTex, "Shininess tex");
	LogTexture(materialData.opacityTex, "Opacity tex");
	LogTexture(materialData.displacementTex, "Displacement tex");
	LogTexture(materialData.lightmapTex, "Light-map tex");
	LogTexture(materialData.reflectionTex, "Reflection tex");
}


class ModelLoader
{
public:
	ModelLoader(IDevice* device, const string& filename, const VertexLayoutBase* layout, float scale, ModelLoad loadFlags, bool loadMaterials);

	ModelPtr Load();

protected:
	void ProcessNode(ModelPtr model, const aiNode* node, const aiScene* scene);
	void ProcessMesh(ModelPtr model, const aiMesh* aiMesh, const aiScene* scene);
	int ProcessMaterial(ModelPtr model, const aiMaterial* aiMaterial, const aiScene* scene);
	TexturePtr FindOrCreateTexture(const aiScene* scene, const string& textureName);
	TexturePtr CreateTexture(const aiScene* scene, const string& textureName);

protected:
	IDevice* m_device{ nullptr };
	string m_filename;
	const VertexLayoutBase* m_vertexLayout{ nullptr };
	float m_scale{ 1.0f };
	ModelLoad m_loadFlags;
	bool m_loadMaterials{ false };

	std::map<std::string, TexturePtr> m_textureCache;
};


ModelLoader::ModelLoader(IDevice* device, const string& filename, const VertexLayoutBase* layout, float scale, ModelLoad loadFlags, bool loadMaterials)
	: m_device{ device }
	, m_filename{ filename }
	, m_vertexLayout{ layout }
	, m_scale{ scale }
	, m_loadFlags{ loadFlags }
	, m_loadMaterials{ loadMaterials }
{}


ModelPtr ModelLoader::Load()
{
	const string fullpath = GetFileSystem()->GetFullPath(m_filename);
	assert(!fullpath.empty());

	Assimp::Importer aiImporter;

	const auto aiScene = aiImporter.ReadFile(fullpath.c_str(), GetPreprocessFlags(m_loadFlags));
	if (!aiScene)
	{
		LogFatal << "Failed to load model file " << m_filename << endl;
		string errorStr = aiImporter.GetErrorString();
		LogFatal << errorStr << endl;
		return nullptr;
	}

	ModelPtr model = make_shared<Model>();

	model->meshes.reserve(aiScene->mNumMeshes);

	ProcessNode(model, aiScene->mRootNode, aiScene);

	return model;
}


void ModelLoader::ProcessNode(ModelPtr model, const aiNode* node, const aiScene* scene)
{
	for (uint32_t i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		ProcessMesh(model, mesh, scene);
	}

	for (uint32_t i = 0; i < node->mNumChildren; i++)
	{
		ProcessNode(model, node->mChildren[i], scene);
	}
}


void ModelLoader::ProcessMesh(ModelPtr model, const aiMesh* aiMesh, const aiScene* aiScene)
{
	uint32_t vertexCount = 0;
	uint32_t indexCount = 0;

	const aiVector3D zero(0.0f, 0.0f, 0.0f);

	const VertexComponent components = m_vertexLayout->GetComponents();

	// Min/max for bounding box computation
	constexpr float maxF = numeric_limits<float>::max();
	Vector3 minExtents{ maxF, maxF, maxF };
	Vector3 maxExtents{ -maxF, -maxF, -maxF };

	vector<float> vertexData;
	vector<float> vertexDataPositionOnly;
	vector<uint32_t> indexData;

	MeshPtr mesh = make_shared<Mesh>();
	MeshPart meshPart{};

	vertexCount += aiMesh->mNumVertices;

	aiColor4D defaultColor(0.0f, 0.0f, 0.0f, 1.0f);
	aiScene->mMaterials[aiMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, defaultColor);

	for (uint32_t j = 0; j < aiMesh->mNumVertices; ++j)
	{
		const aiVector3D* pos = &(aiMesh->mVertices[j]);
		const aiVector3D* normal = &(aiMesh->mNormals[j]);
		const aiVector3D* texCoord = (aiMesh->HasTextureCoords(0)) ? &(aiMesh->mTextureCoords[0][j]) : &zero;
		const aiColor4D* color = (aiMesh->HasVertexColors(0)) ? &(aiMesh->mColors[0][j]) : &defaultColor;
		const aiVector3D* tangent = (aiMesh->HasTangentsAndBitangents()) ? &(aiMesh->mTangents[j]) : &zero;
		const aiVector3D* bitangent = (aiMesh->HasTangentsAndBitangents()) ? &(aiMesh->mBitangents[j]) : &zero;

		if (HasFlag(components, VertexComponent::Position))
		{
			vertexData.push_back(pos->x * m_scale);
			vertexData.push_back(pos->y * m_scale);  // TODO: Is this a hack?
			vertexData.push_back(pos->z * m_scale);

			vertexDataPositionOnly.push_back(pos->x * m_scale);
			vertexDataPositionOnly.push_back(pos->y * m_scale);  // TODO: Is this a hack?
			vertexDataPositionOnly.push_back(pos->z * m_scale);

			UpdateExtents(minExtents, maxExtents, m_scale * Math::Vector3(pos->x, pos->y, pos->z));
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
			vertexData.push_back(color->r);
			vertexData.push_back(color->g);
			vertexData.push_back(color->b);
			vertexData.push_back(color->a);
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
	uint32_t stride = m_vertexLayout->GetSizeInBytes();

	GpuBufferDesc vertexBufferDesc{
		.name = "Model|VertexBuffer",
		.resourceType = ResourceType::VertexBuffer,
		.memoryAccess = MemoryAccess::GpuRead,
		.elementCount = sizeof(float) * vertexData.size() / stride,
		.elementSize = stride,
		.initialData = vertexData.data()
	};
	mesh->vertexBuffer = m_device->CreateGpuBuffer(vertexBufferDesc);

	// Create position-only vertex buffer
	stride = 3 * sizeof(float);
	GpuBufferDesc positionOnlyVertexBufferDesc
	{
		.name = "Model|VertexBuffer (Position Only)",
		.resourceType = ResourceType::VertexBuffer,
		.memoryAccess = MemoryAccess::GpuRead,
		.elementCount = sizeof(float) * vertexDataPositionOnly.size() / stride,
		.elementSize = stride,
		.initialData = vertexDataPositionOnly.data()
	};
	mesh->vertexBufferPositionOnly = m_device->CreateGpuBuffer(positionOnlyVertexBufferDesc);

	// TODO: Support uint16 indices

	// Create index buffer
	GpuBufferDesc indexBufferDesc{
		.name = "Model|IndexBuffer",
		.resourceType = ResourceType::IndexBuffer,
		.memoryAccess = MemoryAccess::GpuRead,
		.elementCount = indexData.size(),
		.elementSize = sizeof(uint32_t),
		.initialData = indexData.data()
	};
	mesh->indexBuffer = m_device->CreateGpuBuffer(indexBufferDesc);

	// Set bounding box
	mesh->boundingBox = Math::BoundingBoxFromMinMax(minExtents, maxExtents);

	mesh->meshParts.push_back(meshPart);

	model->meshes.push_back(mesh);

	// Process materials
	if (m_loadMaterials && aiMesh->mMaterialIndex >= 0)
	{
		::aiMaterial* aiMaterial = aiScene->mMaterials[aiMesh->mMaterialIndex];
		int materialIndex = ProcessMaterial(model, aiMaterial, aiScene);
		mesh->materialIndex = materialIndex;
	}
}


int ModelLoader::ProcessMaterial(ModelPtr model, const aiMaterial* aiMaterial, const aiScene* scene)
{
	MeshMaterialData materialData{};

	aiColor4D aiColor{};
	float aiFloat = 0.0f;
	int aiInt = 0;
	aiString aiString;

	auto GetColor = [aiMaterial](optional<Color>& colorOpt, const char* str, int i0, int i1)
		{
			aiColor4D aiColor{};
			if (AI_SUCCESS == aiGetMaterialColor(aiMaterial, str, i0, i1, &aiColor))
			{
				Color color{ aiColor.r, aiColor.g, aiColor.b, aiColor.a };
				colorOpt = color;
			}
		};

	auto GetFloat = [aiMaterial](optional<float>& floatOpt, const char* str, int i0, int i1)
		{
			float aiFloat = 0.0f;
			if (AI_SUCCESS == aiGetMaterialFloat(aiMaterial, str, i0, i1, &aiFloat))
			{
				floatOpt = aiFloat;
			}
		};

	auto GetInt = [aiMaterial](optional<int>& intOpt, const char* str, int i0, int i1)
		{
			int aiInt = 0;
			if (AI_SUCCESS == aiGetMaterialInteger(aiMaterial, str, i0, i1, &aiInt))
			{
				intOpt = aiInt;
			}
		};

	auto GetString = [aiMaterial](optional<string>& strOpt, const char* str, int i0, int i1)
		{
			::aiString aiString;
			if (AI_SUCCESS == aiGetMaterialString(aiMaterial, str, i0, i1, &aiString))
			{
				strOpt = string{ aiString.C_Str() };
			}
		};

	GetColor(materialData.diffuse, AI_MATKEY_COLOR_DIFFUSE);
	GetColor(materialData.specular, AI_MATKEY_COLOR_SPECULAR);
	GetColor(materialData.ambient, AI_MATKEY_COLOR_AMBIENT);
	GetColor(materialData.emissive, AI_MATKEY_COLOR_EMISSIVE);
	GetColor(materialData.transparent, AI_MATKEY_COLOR_TRANSPARENT);
	GetColor(materialData.reflective, AI_MATKEY_COLOR_REFLECTIVE);
	GetFloat(materialData.opacity, AI_MATKEY_OPACITY);
	GetFloat(materialData.shininess, AI_MATKEY_SHININESS);
	GetFloat(materialData.specularStrength, AI_MATKEY_SHININESS_STRENGTH);
	GetFloat(materialData.transparencyFactor, AI_MATKEY_TRANSPARENCYFACTOR);
	GetFloat(materialData.bumpScaling, AI_MATKEY_BUMPSCALING);
	GetFloat(materialData.reflectivity, AI_MATKEY_REFLECTIVITY);
	GetInt(materialData.twoSided, AI_MATKEY_TWOSIDED);
	GetInt(materialData.shadingModel, AI_MATKEY_SHADING_MODEL);
	GetInt(materialData.wireframe, AI_MATKEY_ENABLE_WIREFRAME);
	GetInt(materialData.blendFunc, AI_MATKEY_BLEND_FUNC);
	GetString(materialData.diffuseTex, AI_MATKEY_TEXTURE_DIFFUSE(0));
	GetString(materialData.specularTex, AI_MATKEY_TEXTURE_SPECULAR(0));
	GetString(materialData.ambientTex, AI_MATKEY_TEXTURE_AMBIENT(0));
	GetString(materialData.emissiveTex, AI_MATKEY_TEXTURE_EMISSIVE(0));
	GetString(materialData.normalTex, AI_MATKEY_TEXTURE_NORMALS(0));
	GetString(materialData.heightTex, AI_MATKEY_TEXTURE_HEIGHT(0));
	GetString(materialData.shininessTex, AI_MATKEY_TEXTURE_SHININESS(0));
	GetString(materialData.opacityTex, AI_MATKEY_TEXTURE_OPACITY(0));
	GetString(materialData.displacementTex, AI_MATKEY_TEXTURE_DISPLACEMENT(0));
	GetString(materialData.lightmapTex, AI_MATKEY_TEXTURE_LIGHTMAP(0));
	GetString(materialData.reflectionTex, AI_MATKEY_TEXTURE_REFLECTION(0));

	//LogMaterialData(materialData);

	TexturePtr diffuseTex;
	if (materialData.diffuseTex.has_value())
	{
		diffuseTex = FindOrCreateTexture(scene, materialData.diffuseTex.value());
	}

	MeshMaterial material{};
	if (materialData.diffuse)
	{
		material.diffuseColor = materialData.diffuse.value();
	}
	material.diffuseTexture = diffuseTex;

	int index = (int)model->materials.size();
	model->materials.push_back(material);

	return index;
}


TexturePtr ModelLoader::FindOrCreateTexture(const aiScene* scene, const string& textureName)
{
	// Check the texture cache to see if we've already created this texture
	auto iter = m_textureCache.find(textureName);
	if (iter != m_textureCache.end())
	{
		return iter->second;
	}

	TexturePtr texture = CreateTexture(scene, textureName);
	m_textureCache[textureName] = texture;

	return texture;
}


TexturePtr ModelLoader::CreateTexture(const aiScene* scene, const string& textureName)
{
	const aiTexture* aiTexture = scene->GetEmbeddedTexture(textureName.c_str());

	if (aiTexture != nullptr)
	{
		string filename = aiTexture->mFilename.C_Str();

		TexturePtr texture = m_device->CreateUninitializedTexture(filename, filename);

		std::byte* data = (std::byte*)aiTexture->pcData;
		size_t dataSize = aiTexture->mWidth;

		CreateTextureFromMemory(m_device, texture.Get(), filename, data, dataSize, Format::Unknown, false);

		return texture;
	}
	else
	{
		LogError(LogModel) << "Can't load external texture " << textureName << endl;
	}

	return nullptr;
}


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


ModelPtr LoadModel(IDevice* device, const string& filename, const VertexLayoutBase& layout, float scale, ModelLoad modelLoadFlags, bool loadMaterials)
{
	ModelLoader loader{ device, filename, &layout, scale, modelLoadFlags, loadMaterials };

	return loader.Load();
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