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

#include "InputLayout.h"

using namespace std;


namespace Luna
{

const char* GetVertexComponentName(VertexComponent component)
{
	static const char* sPosition = "POSITION";
	static const char* sNormal = "NORMAL";
	static const char* sTangent = "TANGENT";
	static const char* sBitangent = "BITANGENT";
	static const char* sColor = "COLOR";
	static const char* sTexcoord = "TEXCOORD";
	static const char* sBlendIndices = "BLENDINDICES";
	static const char* sBlendWeight = "BLENDWEIGHT";

	if (component == VertexComponent::Position)
		return sPosition;

	if (component == VertexComponent::Normal)
		return sNormal;

	if (component == VertexComponent::Tangent)
		return sTangent;

	if (component == VertexComponent::Bitangent)
		return sBitangent;

	// Note Color == Color0, so this handles both enums
	if (component == VertexComponent::Color0 || component == VertexComponent::Color1)
		return sColor;

	if (component == VertexComponent::Texcoord0 ||
		component == VertexComponent::Texcoord1 ||
		component == VertexComponent::Texcoord2 ||
		component == VertexComponent::Texcoord3)
		return sTexcoord;

	if (component == VertexComponent::BlendIndices)
		return sBlendIndices;

	if (component == VertexComponent::BlendWeight)
		return sBlendWeight;

	assert(false);
	return nullptr;
}


uint32_t GetVertexComponentSemantic(VertexComponent component)
{
	if (component == VertexComponent::Color1 || component == VertexComponent::Texcoord1)
		return 1;

	if (component == VertexComponent::Texcoord2)
		return 2;

	if (component == VertexComponent::Texcoord3)
		return 3;

	return 0;
}


Format GetVertexComponentFormat(VertexComponent component)
{
	if (component == VertexComponent::BlendIndices)
		return Format::R32_UInt;

	if (component == VertexComponent::BlendWeight)
		return Format::R32_Float;

	if (component == VertexComponent::Texcoord0 ||
		component == VertexComponent::Texcoord1 ||
		component == VertexComponent::Texcoord2 ||
		component == VertexComponent::Texcoord3)
		return Format::RG32_Float;

	if (component == VertexComponent::Color0 || component == VertexComponent::Color1)
		return Format::RGBA32_Float;

	return Format::RGB32_Float;
}


uint32_t GetVertexComponentSizeInBytes(VertexComponent component)
{
	return sizeof(float) * GetVertexComponentNumFloats(component);
}


uint32_t GetVertexComponentNumFloats(VertexComponent component)
{
	static const VertexComponent OneFloatComponents = VertexComponent::BlendIndices | VertexComponent::BlendWeight;
	static const VertexComponent TwoFloatComponents = VertexComponent::Texcoord0 | VertexComponent::Texcoord1 | VertexComponent::Texcoord2 | VertexComponent::Texcoord3;
	static const VertexComponent ThreeFloatComponents = VertexComponent::Position | VertexComponent::Normal | VertexComponent::Tangent | VertexComponent::Bitangent;
	static const VertexComponent FourFloatComponents = VertexComponent::Color0 | VertexComponent::Color1;

	if (HasAnyFlag(component, OneFloatComponents))
		return 1;
	else if (HasAnyFlag(component, TwoFloatComponents))
		return 2;
	else if (HasAnyFlag(component, ThreeFloatComponents))
		return 3;
	else if (HasAnyFlag(component, FourFloatComponents))
		return 4;

	assert(false);
	return 0;
}


void VertexLayoutBase::Setup(VertexComponent components)
{
	static unordered_map<VertexComponent, uint32_t> cachedSizeInBytes;
	static unordered_map<VertexComponent, uint32_t> cachedNumFloats;
	static unordered_map<VertexComponent, unique_ptr<vector<VertexElementDesc>>> cachedElements;
	static mutex cacheMutex;

	scoped_lock lock(cacheMutex);

	// Find or compute size in bytes
	{
		auto res = cachedSizeInBytes.find(components);
		if (res != cachedSizeInBytes.end())
		{
			m_sizeInBytes = res->second;
		}
		else
		{
			unsigned long index{ 0 };
			unsigned long tempComponents = unsigned long(components);
			while (_BitScanForward(&index, tempComponents))
			{
				VertexComponent singleComponent = VertexComponent(1 << index);
				tempComponents ^= (1 << index);

				m_sizeInBytes += GetVertexComponentSizeInBytes(singleComponent);
			}

			cachedSizeInBytes[components] = m_sizeInBytes;
		}
	}

	// Find or compute num floats
	{
		auto res = cachedNumFloats.find(components);
		if (res != cachedNumFloats.end())
		{
			m_numFloats = res->second;
		}
		else
		{
			unsigned long index{ 0 };
			unsigned long tempComponents = unsigned long(components);
			while (_BitScanForward(&index, tempComponents))
			{
				VertexComponent singleComponent = VertexComponent(1 << index);
				tempComponents ^= (1 << index);

				m_numFloats += GetVertexComponentNumFloats(singleComponent);
			}

			cachedNumFloats[components] = m_numFloats;
		}
	}

	// Find or construct list of VertexElementDescs
	{
		auto res = cachedElements.find(components);
		if (res != cachedElements.end())
		{
			m_elements = res->second.get();
		}
		else
		{
			auto elements = make_unique<vector<VertexElementDesc>>();

			unsigned long index{ 0 };
			unsigned long tempComponents = unsigned long(components);
			uint32_t offset{ 0 };
			while (_BitScanForward(&index, tempComponents))
			{
				VertexComponent singleComponent = VertexComponent(1 << index);
				tempComponents ^= (1 << index);

				VertexElementDesc desc{
					GetVertexComponentName(singleComponent),
					GetVertexComponentSemantic(singleComponent),
					GetVertexComponentFormat(singleComponent),
					0,
					offset,
					InputClassification::PerVertexData,
					0 };

				elements->push_back(desc);

				offset += GetVertexComponentSizeInBytes(singleComponent);
			}

			cachedElements[components] = move(elements);
		}
	}
}

} // namespace Luna