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

#include "Graphics\Enums.h"


namespace Luna
{

class IGpuResource
{ 
public:
	virtual ~IGpuResource() = default;

	ResourceType GetResourceType() const noexcept { return m_type; }
	ResourceState GetUsageState() const noexcept { return m_usageState; }
	void SetUsageState(ResourceState usageState) noexcept { m_usageState = usageState; }

protected:
	ResourceType m_type{ ResourceType::Unknown };
	ResourceState m_usageState{ ResourceState::Common };
};

using GpuResourcePtr = std::shared_ptr<IGpuResource>;

} // namespace Luna
