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

#include "Limits.h"


namespace Luna::Limits
{

uint32_t ConstantBufferAlignment()
{
	return GetLimits()->ConstantBufferAlignment();
}


uint32_t MaxTextureDimension1D()
{
	return GetLimits()->MaxTextureDimension1D();
}


uint32_t MaxTextureDimension2D()
{
	return GetLimits()->MaxTextureDimension2D();
}


uint32_t MaxTextureDimension3D()
{
	return GetLimits()->MaxTextureDimension3D();
}


uint32_t MaxTextureDimensionCube()
{
	return GetLimits()->MaxTextureDimensionCube();
}


uint32_t MaxTexture1DArrayElements()
{
	return GetLimits()->MaxTexture1DArrayElements();
}


uint32_t MaxTexture2DArrayElements()
{
	return GetLimits()->MaxTexture2DArrayElements();
}


uint32_t MaxTextureMipLevels()
{
	return GetLimits()->MaxTextureMipLevels();
}

} // namespace Luna::Limits


Luna::ILimits* g_limits{ nullptr };


namespace Luna
{

const ILimits* GetLimits()
{
	assert(g_limits != nullptr);
	return g_limits;
}

} // namespace Luna