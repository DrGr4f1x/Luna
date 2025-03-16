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


void DepthBuffer::Initialize(const DepthBufferDesc& depthBufferDesc)
{
	m_handle = GetResourceManager()->CreateDepthBuffer(depthBufferDesc);
}


ResourceType DepthBuffer::GetResourceType() const
{
	auto res = GetResourceManager()->GetResourceType(m_handle.get());
	assert(res.has_value());
	return *res;
}


ResourceState DepthBuffer::GetUsageState() const
{
	auto res = GetResourceManager()->GetUsageState(m_handle.get());
	assert(res.has_value());
	return *res;
}


void DepthBuffer::SetUsageState(ResourceState newState)
{
	GetResourceManager()->SetUsageState(m_handle.get(), newState);
}


uint64_t DepthBuffer::GetWidth() const
{
	auto res = GetResourceManager()->GetWidth(m_handle.get());
	assert(res.has_value());
	return *res;
}


uint32_t DepthBuffer::GetHeight() const
{
	auto res = GetResourceManager()->GetHeight(m_handle.get());
	assert(res.has_value());
	return *res;
}


uint32_t DepthBuffer::GetDepth() const
{
	auto res = GetResourceManager()->GetDepthOrArraySize(m_handle.get());
	assert(res.has_value());
	return *res;
}


uint32_t DepthBuffer::GetArraySize() const
{
	auto res = GetResourceManager()->GetDepthOrArraySize(m_handle.get());
	assert(res.has_value());
	return *res;
}


uint32_t DepthBuffer::GetNumMips() const
{
	auto res = GetResourceManager()->GetNumMips(m_handle.get());
	assert(res.has_value());
	return *res;
}


uint32_t DepthBuffer::GetNumSamples() const
{
	auto res = GetResourceManager()->GetNumSamples(m_handle.get());
	assert(res.has_value());
	return *res;
}


uint32_t DepthBuffer::GetPlaneCount() const
{
	auto res = GetResourceManager()->GetPlaneCount(m_handle.get());
	assert(res.has_value());
	return *res;
}


Format DepthBuffer::GetFormat() const
{
	auto res = GetResourceManager()->GetFormat(m_handle.get());
	assert(res.has_value());
	return *res;
}


TextureDimension DepthBuffer::GetDimension() const
{
	return ResourceTypeToTextureDimension(GetResourceType());
}


float DepthBuffer::GetClearDepth() const
{
	auto res = GetResourceManager()->GetClearDepth(m_handle.get());
	assert(res.has_value());
	return *res;
}


uint8_t DepthBuffer::GetClearStencil() const
{
	auto res = GetResourceManager()->GetClearStencil(m_handle.get());
	assert(res.has_value());
	return *res;
}

} // namespace Luna