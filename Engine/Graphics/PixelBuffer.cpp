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

#include "Enums.h"
#include "Formats.h"


namespace Luna
{

PixelBuffer::PixelBuffer() noexcept
	: m_format{ Format::Unknown }
{
}


uint32_t PixelBuffer::GetDepth() const noexcept
{ 
	return m_resourceType == ResourceType::Texture3D ? m_arraySizeOrDepth : 1; 
}


uint32_t PixelBuffer::GetArraySize() const noexcept
{ 
	return m_resourceType == ResourceType::Texture3D ? 1 : m_arraySizeOrDepth; 
}


TextureDimension PixelBuffer::GetDimension() const noexcept
{ 
	return ResourceTypeToTextureDimension(m_resourceType); 
}


void PixelBuffer::Reset() noexcept
{
	m_width = 0;
	m_height = 0;
	m_arraySizeOrDepth = 0;
	m_numMips = 1;
	m_numSamples = 1;
	m_planeCount = 1;
	m_format = Format::Unknown;

	GpuResource::Reset();
}

} // namespace Luna