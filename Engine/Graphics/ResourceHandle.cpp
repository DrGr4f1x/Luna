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

#include "ResourceHandle.h"

#include "ResourceManager.h"


namespace Luna
{

ResourceHandleType::~ResourceHandleType()
{
	assert(m_manager);
	m_manager->DestroyHandle(this);
}

} // namespace Luna