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

#include "Graphics\DeviceManager.h"


namespace Luna::DX12
{

class DeviceManager12 : public DeviceManager, public NonCopyable
{
	IMPLEMENT_IOBJECT

public:
	virtual ~DeviceManager12() = default;
};

} // namespace Luna::DX12