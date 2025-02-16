//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#include "StdAfx.h"

#include "ColorBuffer.h"

#include "GraphicsCommon.h"


namespace Luna
{

ColorBufferHandleType::~ColorBufferHandleType()
{
	assert(m_pool);
	m_pool->DestroyHandle(this);
}


void ColorBuffer::Initialize(const ColorBufferDesc& ColorBufferDesc)
{
	m_handle = GetColorBufferPool()->CreateColorBuffer(ColorBufferDesc);
}


ResourceType ColorBuffer::GetResourceType() const
{
	return GetColorBufferPool()->GetResourceType(m_handle.get());
}


ResourceState ColorBuffer::GetUsageState() const
{
	return GetColorBufferPool()->GetUsageState(m_handle.get());
}


void ColorBuffer::SetUsageState(ResourceState newState)
{
	GetColorBufferPool()->SetUsageState(m_handle.get(), newState);
}


uint64_t ColorBuffer::GetWidth() const
{
	return GetColorBufferPool()->GetWidth(m_handle.get());
}


uint32_t ColorBuffer::GetHeight() const
{
	return GetColorBufferPool()->GetHeight(m_handle.get());
}


uint32_t ColorBuffer::GetDepth() const
{
	return GetColorBufferPool()->GetDepthOrArraySize(m_handle.get());
}


uint32_t ColorBuffer::GetArraySize() const
{
	return GetColorBufferPool()->GetDepthOrArraySize(m_handle.get());
}


uint32_t ColorBuffer::GetNumMips() const
{
	return GetColorBufferPool()->GetNumMips(m_handle.get());
}


uint32_t ColorBuffer::GetNumSamples() const
{
	return GetColorBufferPool()->GetNumSamples(m_handle.get());
}


uint32_t ColorBuffer::GetPlaneCount() const
{
	return GetColorBufferPool()->GetPlaneCount(m_handle.get());
}


Format ColorBuffer::GetFormat() const
{
	return GetColorBufferPool()->GetFormat(m_handle.get());
}


TextureDimension ColorBuffer::GetDimension() const
{
	return ResourceTypeToTextureDimension(GetResourceType());
}


Color ColorBuffer::GetClearColor() const
{
	return GetColorBufferPool()->GetClearColor(m_handle.get());
}

} // namespace Luna