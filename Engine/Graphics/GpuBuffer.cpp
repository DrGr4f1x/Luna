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

#include "GpuBuffer.h"

#include "CommandContext.h"
#include "GraphicsCommon.h"


namespace Luna
{

GpuBufferHandleType::~GpuBufferHandleType()
{
	assert(m_pool);
	m_pool->DestroyHandle(this);
}


void GpuBuffer::Initialize(const GpuBufferDesc& gpuBufferDesc)
{
	m_handle = GetGpuBufferPool()->CreateGpuBuffer(gpuBufferDesc);

	if (gpuBufferDesc.initialData != nullptr)
	{
		CommandContext::InitializeBuffer(*this, gpuBufferDesc.initialData, GetSize());
	}
}


ResourceType GpuBuffer::GetResourceType() const
{
	return GetGpuBufferPool()->GetResourceType(m_handle.get());
}


ResourceState GpuBuffer::GetUsageState() const
{
	return GetGpuBufferPool()->GetUsageState(m_handle.get());
}


void GpuBuffer::SetUsageState(ResourceState newState)
{
	GetGpuBufferPool()->SetUsageState(m_handle.get(), newState);
}


size_t GpuBuffer::GetSize() const
{
	return GetGpuBufferPool()->GetSize(m_handle.get());
}



size_t GpuBuffer::GetElementCount() const
{
	return GetGpuBufferPool()->GetElementCount(m_handle.get());
}


size_t GpuBuffer::GetElementSize() const
{
	return GetGpuBufferPool()->GetElementSize(m_handle.get());
}


void GpuBuffer::Update(size_t sizeInBytes, const void* data)
{
	GetGpuBufferPool()->Update(m_handle.get(), sizeInBytes, 0, data);
}

} // namespace Luna