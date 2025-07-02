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

namespace Limits
{

uint32_t ConstantBufferAlignment();
uint32_t MaxTextureDimension1D();
uint32_t MaxTextureDimension2D();
uint32_t MaxTextureDimension3D();
uint32_t MaxTextureDimensionCube();
uint32_t MaxTexture1DArrayElements();
uint32_t MaxTexture2DArrayElements();
uint32_t MaxTextureMipLevels();

} // namespace Limits

class ILimits
{
public:
	virtual ~ILimits() = default;

	virtual uint32_t ConstantBufferAlignment() const = 0;
	virtual uint32_t MaxTextureDimension1D() const = 0;
	virtual uint32_t MaxTextureDimension2D() const = 0;
	virtual uint32_t MaxTextureDimension3D() const = 0;
	virtual uint32_t MaxTextureDimensionCube() const = 0;
	virtual uint32_t MaxTexture1DArrayElements() const = 0;
	virtual uint32_t MaxTexture2DArrayElements() const = 0;
	virtual uint32_t MaxTextureMipLevels() const = 0;
};

const ILimits* GetLimits();

} // namespace Luna