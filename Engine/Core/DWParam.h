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
	DWParam(float f) : f_value{ f } {}
	DWParam(uint32_t u) : u_value{ u } {}
	DWParam(int32_t i) : i_value{ i } {}

	void operator=(float f) { f_value = f; }
	void operator=(uint32_t u) { u_value = u; }
	void operator=(int32_t i) { i_value = i; }

	union
	{
		float f_value;
		uint32_t u_value;
		int32_t i_value;
	};
};

} // namespace Luna