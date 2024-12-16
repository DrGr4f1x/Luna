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


namespace Luna::VK
{

class DeviceManager : public Luna::DeviceManager, public NonCopyable
{
	IMPLEMENT_IOBJECT

public:
	DeviceManager(const DeviceManagerDesc& desc);
	virtual ~DeviceManager() = default;

protected:
	void Initialize();

private:
	DeviceManagerDesc m_desc{};
	bool m_bIsDeveloperModeEnabled{ false };
	bool m_bIsRenderDocAvailable{ false };
};

} // namespace Luna::VK