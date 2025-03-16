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

void GpuBuffer::Initialize(const GpuBufferDesc& gpuBufferDesc)
{
	m_handle = GetResourceManager()->CreateGpuBuffer(gpuBufferDesc);

	if (gpuBufferDesc.initialData != nullptr)
	{
		CommandContext::InitializeBuffer(*this, gpuBufferDesc.initialData, GetSize());
	}
}


ResourceType GpuBuffer::GetResourceType() const
{
	auto res = GetResourceManager()->GetResourceType(m_handle.get());
	assert(res.has_value());
	return *res;
}


ResourceState GpuBuffer::GetUsageState() const
{
	auto res = GetResourceManager()->GetUsageState(m_handle.get());
	assert(res.has_value());
	return *res;
}


void GpuBuffer::SetUsageState(ResourceState newState)
{
	GetResourceManager()->SetUsageState(m_handle.get(), newState);
}


size_t GpuBuffer::GetSize() const
{
	auto res = GetResourceManager()->GetSize(m_handle.get());
	assert(res.has_value());
	return *res;
}



size_t GpuBuffer::GetElementCount() const
{
	auto res = GetResourceManager()->GetElementCount(m_handle.get());
	assert(res.has_value());
	return *res;
}


size_t GpuBuffer::GetElementSize() const
{
	auto res = GetResourceManager()->GetElementSize(m_handle.get());
	assert(res.has_value());
	return *res;
}


void GpuBuffer::Update(size_t sizeInBytes, const void* data)
{
	GetResourceManager()->Update(m_handle.get(), sizeInBytes, 0, data);
}


void GpuBuffer::Update(size_t sizeInBytes, size_t offset, const void* data)
{
	GetResourceManager()->Update(m_handle.get(), sizeInBytes, offset, data);
}

} // namespace Luna