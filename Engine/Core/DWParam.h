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

struct DWParam
{
	DWParam(float f) : value{ f } {}
	DWParam(uint32_t u) : value{ u } {}
	DWParam(int32_t i) : value{ i } {}

	void operator=(float f) { value = f; }
	void operator=(uint32_t u) { value = u; }
	void operator=(int32_t i) { value = i; }

	std::variant<float, uint32_t, int32_t> value;
};

} // namespace Luna