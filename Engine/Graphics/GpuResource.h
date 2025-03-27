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

#include "Graphics\ResourceManager.h"


namespace Luna
{

// Forward declarations
enum class ResourceState : uint32_t;
enum class ResourceType : uint32_t;


class GpuResource
{
public:
	virtual ~GpuResource() = default;

	ResourceType GetResourceType() const;
	ResourceState GetUsageState() const;
	void SetUsageState(ResourceState usageState);

	ResourceHandle GetHandle() const { return m_handle; }

protected:
	ResourceHandle m_handle;
};

} // namespace Luna
