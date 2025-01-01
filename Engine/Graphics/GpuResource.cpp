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

#include "Enums.h"


namespace Luna
{

GpuResource::GpuResource() noexcept
{
	Reset();
}


ResourceState GpuResource::GetUsageState() const noexcept
{ 
	return m_usageState; 
}


void GpuResource::SetUsageState(ResourceState usageState) noexcept
{ 
	m_usageState = usageState; 
}


ResourceState GpuResource::GetTransitioningState() const noexcept
{ 
	return m_transitioningState; 
}


void GpuResource::SetTransitioningState(ResourceState transitioningState) noexcept
{ 
	m_transitioningState = transitioningState; 
}


ResourceType GpuResource::GetResourceType() const noexcept
{ 
	return m_resourceType; 
}


void GpuResource::Reset() noexcept
{
	m_name = "";
	m_platformData.reset();
	m_usageState = ResourceState::Undefined;
	m_transitioningState = ResourceState::Undefined;
	m_resourceType = ResourceType::Unknown;
}

} // namespace Luna