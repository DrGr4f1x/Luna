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
	assert(m_manager);
	m_manager->DestroyHandle(this);
}


void ColorBuffer::Initialize(const ColorBufferDesc& ColorBufferDesc)
{
	m_handle = GetColorBufferManager()->CreateColorBuffer(ColorBufferDesc);
}


ResourceType ColorBuffer::GetResourceType() const
{
	return GetColorBufferManager()->GetResourceType(m_handle.get());
}


ResourceState ColorBuffer::GetUsageState() const
{
	return GetColorBufferManager()->GetUsageState(m_handle.get());
}


void ColorBuffer::SetUsageState(ResourceState newState)
{
	GetColorBufferManager()->SetUsageState(m_handle.get(), newState);
}


uint64_t ColorBuffer::GetWidth() const
{
	return GetColorBufferManager()->GetWidth(m_handle.get());
}


uint32_t ColorBuffer::GetHeight() const
{
	return GetColorBufferManager()->GetHeight(m_handle.get());
}


uint32_t ColorBuffer::GetDepth() const
{
	return GetColorBufferManager()->GetDepthOrArraySize(m_handle.get());
}


uint32_t ColorBuffer::GetArraySize() const
{
	return GetColorBufferManager()->GetDepthOrArraySize(m_handle.get());
}


uint32_t ColorBuffer::GetNumMips() const
{
	return GetColorBufferManager()->GetNumMips(m_handle.get());
}


uint32_t ColorBuffer::GetNumSamples() const
{
	return GetColorBufferManager()->GetNumSamples(m_handle.get());
}


uint32_t ColorBuffer::GetPlaneCount() const
{
	return GetColorBufferManager()->GetPlaneCount(m_handle.get());
}


Format ColorBuffer::GetFormat() const
{
	return GetColorBufferManager()->GetFormat(m_handle.get());
}


TextureDimension ColorBuffer::GetDimension() const
{
	return ResourceTypeToTextureDimension(GetResourceType());
}


Color ColorBuffer::GetClearColor() const
{
	return GetColorBufferManager()->GetClearColor(m_handle.get());
}

} // namespace Luna