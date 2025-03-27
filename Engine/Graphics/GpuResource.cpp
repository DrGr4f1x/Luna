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

#include "GpuResource.h"

#include "GraphicsCommon.h"


namespace Luna
{

ResourceType GpuResource::GetResourceType() const
{
	auto res = GetResourceManager()->GetResourceType(m_handle.get());
	assert(res.has_value());
	return *res;
}


ResourceState GpuResource::GetUsageState() const
{
	auto res = GetResourceManager()->GetUsageState(m_handle.get());
	assert(res.has_value());
	return *res;
}


void GpuResource::SetUsageState(ResourceState usageState)
{
	GetResourceManager()->SetUsageState(m_handle.get(), usageState);
}

} // namespace Luna