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
	m_handle = GetDepthBufferManager()->CreateDepthBuffer(depthBufferDesc);
}


ResourceType DepthBuffer::GetResourceType() const
{
	return GetDepthBufferManager()->GetResourceType(m_handle.get());
}


ResourceState DepthBuffer::GetUsageState() const
{
	return GetDepthBufferManager()->GetUsageState(m_handle.get());
}


void DepthBuffer::SetUsageState(ResourceState newState)
{
	GetDepthBufferManager()->SetUsageState(m_handle.get(), newState);
}


uint64_t DepthBuffer::GetWidth() const
{
	return GetDepthBufferManager()->GetWidth(m_handle.get());
}


uint32_t DepthBuffer::GetHeight() const
{
	return GetDepthBufferManager()->GetHeight(m_handle.get());
}


uint32_t DepthBuffer::GetDepth() const
{
	return GetDepthBufferManager()->GetDepthOrArraySize(m_handle.get());
}


uint32_t DepthBuffer::GetArraySize() const
{
	return GetDepthBufferManager()->GetDepthOrArraySize(m_handle.get());
}


uint32_t DepthBuffer::GetNumMips() const
{
	return GetDepthBufferManager()->GetNumMips(m_handle.get());
}


uint32_t DepthBuffer::GetNumSamples() const
{
	return GetDepthBufferManager()->GetNumSamples(m_handle.get());
}


uint32_t DepthBuffer::GetPlaneCount() const
{
	return GetDepthBufferManager()->GetPlaneCount(m_handle.get());
}


Format DepthBuffer::GetFormat() const
{
	return GetDepthBufferManager()->GetFormat(m_handle.get());
}


TextureDimension DepthBuffer::GetDimension() const
{
	return ResourceTypeToTextureDimension(GetResourceType());
}


float DepthBuffer::GetClearDepth() const
{
	return GetDepthBufferManager()->GetClearDepth(m_handle.get());
}


uint8_t DepthBuffer::GetClearStencil() const
{
	return GetDepthBufferManager()->GetClearStencil(m_handle.get());
}

} // namespace Luna