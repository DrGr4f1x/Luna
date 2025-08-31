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

#include "GraphicsFeatures.h"

using namespace std;


namespace Luna
{

GraphicsFeatureSet g_requiredFeatures;
GraphicsFeatureSet g_optionalFeatures;
const GraphicsFeatureSet g_enabledFeatures;


GraphicsFeatureProxy::GraphicsFeatureProxy(GraphicsFeatureSet* featureSet, GraphicsFeature feature)
	: m_feature{ feature }
{
	featureSet->RegisterFeature(this);
}


const string GraphicsFeatureProxy::GetName() const
{
	return GraphicsFeatureToString(m_feature);
}


void GraphicsFeatureSet::RegisterFeature(GraphicsFeatureProxy* featureProxy)
{
	m_features.push_back(featureProxy);
}


const string GraphicsFeatureToString(GraphicsFeature feature)
{
	static vector<string> s_stringTable =
	{
		"Mesh Shader",
		"Bindless"
	};
	
	return s_stringTable[(uint32_t)feature];
}

} // namespace Luna