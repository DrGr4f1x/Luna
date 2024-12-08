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

#include "Application.h"

#include "Core\Utility.h"

namespace Luna
{

IApplication::IApplication(uint32_t width, uint32_t height, const std::string& appTitle)
	: m_width{ width }
	, m_height{ height }
	, m_appTitle{ appTitle }
{}


const std::wstring IApplication::GetWTitle() const
{
	return MakeWStr(m_appTitle);
}

} // namespace Luna