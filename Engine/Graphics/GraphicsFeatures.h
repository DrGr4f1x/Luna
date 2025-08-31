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

namespace Luna
{

// Forward declarations
class GraphicsFeatureSet;


enum class GraphicsFeature
{
	MeshShader,
	Bindless
};


class GraphicsFeatureProxy
{
public:
	GraphicsFeatureProxy(GraphicsFeatureSet* featureSet, GraphicsFeature feature);

	operator bool() const { return m_enabled; }

	GraphicsFeatureProxy& operator=(bool enabled)
	{
		m_enabled = enabled;
		return *this;
	}

	GraphicsFeature GetFeature() const { return m_feature; }

	const std::string GetName() const;

protected:
	const GraphicsFeature m_feature;
	bool m_enabled{ false };
};


class GraphicsFeatureSet : NonCopyable
{
	friend class GraphicsFeatureProxy;

public:
	GraphicsFeatureProxy meshShader{ this, GraphicsFeature::MeshShader };
	GraphicsFeatureProxy bindless{ this, GraphicsFeature::Bindless };

	const GraphicsFeatureProxy& operator[](size_t index) const { return *m_features[index]; }
	GraphicsFeatureProxy& operator[](size_t index) { return *m_features[index]; }

	size_t GetNumFeatures() const { return m_features.size(); }

protected:
	void RegisterFeature(GraphicsFeatureProxy* featureProxy);

protected:
	std::vector<GraphicsFeatureProxy*> m_features;
};

extern GraphicsFeatureSet g_requiredFeatures;
extern GraphicsFeatureSet g_optionalFeatures;
extern const GraphicsFeatureSet g_enabledFeatures;

const std::string GraphicsFeatureToString(GraphicsFeature feature);

} // namespace Luna