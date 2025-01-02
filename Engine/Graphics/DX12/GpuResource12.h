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


namespace Luna::DX12
{

class __declspec(uuid("A51DF8E6-61C4-42DE-A64F-79A9A6480528")) IGpuResourceData : public IPlatformData
{
public:
	virtual ID3D12Resource* GetResource() const noexcept = 0;
};

} // namespace Luna::DX12