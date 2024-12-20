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

#include "Core\IObject.h"

namespace Luna
{

class __declspec(uuid("7910A354-F69A-4A9C-9D69-816A27E68BB6")) IGpuImage : public IUnknown
{
public:
	virtual ~IGpuImage() = default;

	virtual ResourceState GetUsageState() const noexcept = 0;
	virtual void SetUsageState(ResourceState usageState) noexcept = 0;
	virtual ResourceState GetTransitioningState() const noexcept = 0;
	virtual void SetTransitioningState(ResourceState transitioningState) noexcept = 0;
	virtual ResourceType GetResourceType() const noexcept = 0;

	virtual NativeObjectPtr GetNativeObject(NativeObjectType nativeObjectType) const noexcept { (void)nativeObjectType; return nullptr; }
};

} // namespace Luna