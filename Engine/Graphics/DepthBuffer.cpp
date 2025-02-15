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

#include "DepthBuffer.h"

#include "GraphicsCommon.h"


namespace Luna
{

DepthBufferHandleType::~DepthBufferHandleType()
{
	assert(m_pool);
	m_pool->DestroyHandle(this);
}


void DepthBuffer::Initialize(const DepthBufferDesc& depthBufferDesc)
{
	m_handle = GetDepthBufferPool()->CreateDepthBuffer(depthBufferDesc);
}


ResourceType DepthBuffer::GetResourceType() const
{
	return GetDepthBufferPool()->GetResourceType(m_handle.get());
}


ResourceState DepthBuffer::GetUsageState() const
{
	return GetDepthBufferPool()->GetUsageState(m_handle.get());
}


void DepthBuffer::SetUsageState(ResourceState newState)
{
	GetDepthBufferPool()->SetUsageState(m_handle.get(), newState);
}


uint64_t DepthBuffer::GetWidth() const
{
	return GetDepthBufferPool()->GetWidth(m_handle.get());
}


uint32_t DepthBuffer::GetHeight() const
{
	return GetDepthBufferPool()->GetHeight(m_handle.get());
}


uint32_t DepthBuffer::GetDepth() const
{
	return GetDepthBufferPool()->GetDepthOrArraySize(m_handle.get());
}


uint32_t DepthBuffer::GetArraySize() const
{
	return GetDepthBufferPool()->GetDepthOrArraySize(m_handle.get());
}


uint32_t DepthBuffer::GetNumMips() const
{
	return GetDepthBufferPool()->GetNumMips(m_handle.get());
}


uint32_t DepthBuffer::GetNumSamples() const
{
	return GetDepthBufferPool()->GetNumSamples(m_handle.get());
}


uint32_t DepthBuffer::GetPlaneCount() const
{
	return GetDepthBufferPool()->GetPlaneCount(m_handle.get());
}


Format DepthBuffer::GetFormat() const
{
	return GetDepthBufferPool()->GetFormat(m_handle.get());
}


TextureDimension DepthBuffer::GetDimension() const
{
	return ResourceTypeToTextureDimension(GetResourceType());
}


float DepthBuffer::GetClearDepth() const
{
	return GetDepthBufferPool()->GetClearDepth(m_handle.get());
}


uint8_t DepthBuffer::GetClearStencil() const
{
	return GetDepthBufferPool()->GetClearStencil(m_handle.get());
}

} // namespace Luna