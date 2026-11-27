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

#include "ShaderBindingTable.h"

#include <algorithm>

namespace Luna
{

size_t ShaderBindingTableDesc::GetRayGenDataSize() const
{
	if (rayGenShaders.size() > 0)
	{
		auto res = std::max_element(rayGenShaders.begin(), rayGenShaders.end(), [](const ShaderRecord& a, const ShaderRecord& b)
			{
				return std::max(a.size, b.size);
			});
		return res->size;
	}
	return 0;
}


size_t ShaderBindingTableDesc::GetMissDataSize() const
{
	if (missShaders.size() > 0)
	{
		auto res = std::max_element(missShaders.begin(), missShaders.end(), [](const ShaderRecord& a, const ShaderRecord& b)
			{
				return std::max(a.size, b.size);
			});
		return res->size;
	}
	return 0;
}


size_t ShaderBindingTableDesc::GetCallableDataSize() const
{
	if (callableShaders.size() > 0)
	{
		auto res = std::max_element(callableShaders.begin(), callableShaders.end(), [](const ShaderRecord& a, const ShaderRecord& b)
			{
				return std::max(a.size, b.size);
			});
		return res->size;
	}
	return 0;
}


size_t ShaderBindingTableDesc::GetHitGroupDataSize() const
{
	if (hitGroups.size() > 0)
	{
		auto res = std::max_element(hitGroups.begin(), hitGroups.end(), [](const ShaderRecord& a, const ShaderRecord& b)
			{
				return std::max(a.size, b.size);
			});
		return res->size;
	}
	return 0;
}

} // namespace Luna