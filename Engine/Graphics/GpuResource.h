//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#pragma once

#include "Graphics\Resource.h"


namespace Luna
{

// Forward declarations
enum class ResourceState : uint32_t;
enum class ResourceType : uint32_t;


class IGpuResource : public IResource
{ 
public:
	ResourceType GetResourceType() const noexcept { return m_type; }
	ResourceState GetUsageState() const noexcept { return m_usageState; }
	void SetUsageState(ResourceState usageState) noexcept { m_usageState = usageState; }

protected:
	ResourceType m_type{ ResourceType::Unknown };
	ResourceState m_usageState{ ResourceState::Common };
};

using GpuResourcePtr = std::shared_ptr<IGpuResource>;

} // namespace Luna
