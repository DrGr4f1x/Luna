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

class FlagStringMap32
{
public:
	FlagStringMap32(std::initializer_list<std::pair<uint32_t, std::string>> flagStringMap, uint32_t startIndex = 0)
		: m_flagStringMap{ flagStringMap }
		, m_startIndex{ startIndex }
	{
	}

	std::string BuildString(uint32_t flags, char separator = ',') const;

private:
	std::vector<std::pair<uint32_t, std::string>> m_flagStringMap;
	uint32_t m_startIndex{ 0 };
};

} // namespace Kodiak
