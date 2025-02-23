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
	assert(m_manager);
	m_manager->DestroyHandle(this);
}


void GpuBuffer::Initialize(const GpuBufferDesc& gpuBufferDesc)
{
	m_handle = GetGpuBufferManager()->CreateGpuBuffer(gpuBufferDesc);

	if (gpuBufferDesc.initialData != nullptr)
	{
		CommandContext::InitializeBuffer(*this, gpuBufferDesc.initialData, GetSize());
	}
}


ResourceType GpuBuffer::GetResourceType() const
{
	return GetGpuBufferManager()->GetResourceType(m_handle.get());
}


ResourceState GpuBuffer::GetUsageState() const
{
	return GetGpuBufferManager()->GetUsageState(m_handle.get());
}


void GpuBuffer::SetUsageState(ResourceState newState)
{
	GetGpuBufferManager()->SetUsageState(m_handle.get(), newState);
}


size_t GpuBuffer::GetSize() const
{
	return GetGpuBufferManager()->GetSize(m_handle.get());
}



size_t GpuBuffer::GetElementCount() const
{
	return GetGpuBufferManager()->GetElementCount(m_handle.get());
}


size_t GpuBuffer::GetElementSize() const
{
	return GetGpuBufferManager()->GetElementSize(m_handle.get());
}


void GpuBuffer::Update(size_t sizeInBytes, const void* data)
{
	GetGpuBufferManager()->Update(m_handle.get(), sizeInBytes, 0, data);
}


void GpuBuffer::Update(size_t sizeInBytes, size_t offset, const void* data)
{
	GetGpuBufferManager()->Update(m_handle.get(), sizeInBytes, offset, data);
}

} // namespace Luna