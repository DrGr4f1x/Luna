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

namespace Luna
{

// Forward declarations
enum class ResourceState : uint32_t;
enum class ResourceType : uint32_t;


class __declspec(uuid("D1269B10-4D09-4FDD-AB9B-1FDC9EE928E0")) IGpuResource : public IUnknown
{
public:
	virtual ~IGpuResource() = default;

	virtual const std::string& GetName() const = 0;
	virtual ResourceType GetResourceType() const noexcept = 0;

	virtual ResourceState GetUsageState() const noexcept = 0;
	virtual void SetUsageState(ResourceState usageState) noexcept = 0;
	virtual ResourceState GetTransitioningState() const noexcept = 0;
	virtual void SetTransitioningState(ResourceState transitioningState) noexcept = 0;

	virtual NativeObjectPtr GetNativeObject(NativeObjectType type, uint32_t index = 0) const noexcept = 0;
};

using GpuResourceHandle = wil::com_ptr<IGpuResource>;

} // namespace Luna
