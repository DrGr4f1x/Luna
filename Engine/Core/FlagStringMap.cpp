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

#include "FlagStringMap.h"

using namespace std;


namespace Luna
{

string FlagStringMap32::BuildString(uint32_t flags, char separator) const
{
	const uint32_t numFlags = __popcnt(flags);

	std::string result;
	bool bStringEmitted{ false };

	// First, handle the case we have a flag equal to 0, for "None", "Unknown" or similar
	for (const auto& bitKey : m_flagStringMap)
	{
		if (bitKey.first == 0u && bitKey.first == flags)
		{
			return bitKey.second;
		}
	}

	// Next, look for any flags that have more than one bit set (i.e. FlagAB = FlagA | FlagB)
	for (const auto& bitKey : m_flagStringMap)
	{
		const uint32_t numFlagBits = __popcnt(bitKey.first);
		if (numFlagBits > 1 && (flags & bitKey.first) == bitKey.first)
		{
			if (bStringEmitted)
			{
				result += format("{} {}", separator, bitKey.second);
			}
			else
			{
				result += bitKey.second;
				bStringEmitted = true;
			}

			flags ^= bitKey.first;
		}
	}

	// Finally, handle all the flags that have only 1 bit set
	for (const auto& bitKey : m_flagStringMap)
	{
		// Handle the other non-zero flags
		if ((bitKey.first != 0u) && (flags & bitKey.first) == bitKey.first)
		{
			if (bStringEmitted)
			{
				result += format("{} {}", separator, bitKey.second);
			}
			else
			{
				result += bitKey.second;
				bStringEmitted = true;
			}
		}
	}

	return result;
}

} // namespace Luna