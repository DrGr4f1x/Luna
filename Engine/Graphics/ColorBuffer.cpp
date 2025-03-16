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


void ColorBuffer::Initialize(const ColorBufferDesc& ColorBufferDesc)
{
	m_handle = GetResourceManager()->CreateColorBuffer(ColorBufferDesc);
}


ResourceType ColorBuffer::GetResourceType() const
{
	auto res = GetResourceManager()->GetResourceType(m_handle.get());
	assert(res.has_value());
	return *res;
}


ResourceState ColorBuffer::GetUsageState() const
{
	auto res = GetResourceManager()->GetUsageState(m_handle.get());
	assert(res.has_value());
	return *res;
}


void ColorBuffer::SetUsageState(ResourceState newState)
{
	GetResourceManager()->SetUsageState(m_handle.get(), newState);
}


uint64_t ColorBuffer::GetWidth() const
{
	auto res = GetResourceManager()->GetWidth(m_handle.get());
	assert(res.has_value());
	return *res;
}


uint32_t ColorBuffer::GetHeight() const
{
	auto res = GetResourceManager()->GetHeight(m_handle.get());
	assert(res.has_value());
	return *res;
}


uint32_t ColorBuffer::GetDepth() const
{
	auto res = GetResourceManager()->GetDepthOrArraySize(m_handle.get());
	assert(res.has_value());
	return *res;
}


uint32_t ColorBuffer::GetArraySize() const
{
	auto res = GetResourceManager()->GetDepthOrArraySize(m_handle.get());
	assert(res.has_value());
	return *res;
}


uint32_t ColorBuffer::GetNumMips() const
{
	auto res = GetResourceManager()->GetNumMips(m_handle.get());
	assert(res.has_value());
	return *res;
}


uint32_t ColorBuffer::GetNumSamples() const
{
	auto res = GetResourceManager()->GetNumSamples(m_handle.get());
	assert(res.has_value());
	return *res;
}


uint32_t ColorBuffer::GetPlaneCount() const
{
	auto res = GetResourceManager()->GetPlaneCount(m_handle.get());
	assert(res.has_value());
	return *res;
}


Format ColorBuffer::GetFormat() const
{
	auto res = GetResourceManager()->GetFormat(m_handle.get());
	assert(res.has_value());
	return *res;
}


TextureDimension ColorBuffer::GetDimension() const
{
	return ResourceTypeToTextureDimension(GetResourceType());
}


Color ColorBuffer::GetClearColor() const
{
	auto res = GetResourceManager()->GetClearColor(m_handle.get());
	assert(res.has_value());
	return *res;
}


void ColorBuffer::SetHandle(ResourceHandleType* handle) 
{ 
	m_handle = handle; 
}

} // namespace Luna