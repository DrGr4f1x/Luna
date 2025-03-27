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

#include "PixelBuffer.h"

#include "GraphicsCommon.h"


namespace Luna
{

uint64_t PixelBuffer::GetWidth() const
{
	auto res = GetResourceManager()->GetWidth(m_handle.get());
	assert(res.has_value());
	return *res;
}


uint32_t PixelBuffer::GetHeight() const
{
	auto res = GetResourceManager()->GetHeight(m_handle.get());
	assert(res.has_value());
	return *res;
}


uint32_t PixelBuffer::GetDepth() const
{
	auto res = GetResourceManager()->GetDepthOrArraySize(m_handle.get());
	assert(res.has_value());
	return *res;
}


uint32_t PixelBuffer::GetArraySize() const
{
	auto res = GetResourceManager()->GetDepthOrArraySize(m_handle.get());
	assert(res.has_value());
	return *res;
}


uint32_t PixelBuffer::GetNumMips() const
{
	auto res = GetResourceManager()->GetNumMips(m_handle.get());
	assert(res.has_value());
	return *res;
}


uint32_t PixelBuffer::GetNumSamples() const
{
	auto res = GetResourceManager()->GetNumSamples(m_handle.get());
	assert(res.has_value());
	return *res;
}


uint32_t PixelBuffer::GetPlaneCount() const
{
	auto res = GetResourceManager()->GetPlaneCount(m_handle.get());
	assert(res.has_value());
	return *res;
}


Format PixelBuffer::GetFormat() const
{
	auto res = GetResourceManager()->GetFormat(m_handle.get());
	assert(res.has_value());
	return *res;
}


TextureDimension PixelBuffer::GetDimension() const
{
	return ResourceTypeToTextureDimension(GetResourceType());
}

} // namespace Luna