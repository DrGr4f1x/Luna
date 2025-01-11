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

#include "Graphics\PlatformData.h"


namespace Luna
{

// Forward declarations
enum class ResourceState : uint32_t;
enum class ResourceType : uint32_t;


class GpuResource
{
public:
	GpuResource() noexcept;
	virtual ~GpuResource() = default;

	const std::string& GetName() const { return m_name; }

	IPlatformData* GetPlatformData() const noexcept { return m_platformData.get(); }

	ResourceState GetUsageState() const noexcept;
	void SetUsageState(ResourceState usageState) noexcept;
	ResourceState GetTransitioningState() const noexcept;
	void SetTransitioningState(ResourceState transitioningState) noexcept;
	ResourceType GetResourceType() const noexcept;

	void Reset() noexcept;

protected:
	std::string m_name;

	wil::com_ptr<IPlatformData> m_platformData;
	ResourceState m_usageState;
	ResourceState m_transitioningState;
	ResourceType m_resourceType;
};

} // namespace Luna
