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

#include "Graphics\Enums.h"


namespace Luna
{

struct ShaderDesc
{
	std::string filename;
	std::string entry{ "main" };
	ShaderType type{ ShaderType::None };

	ShaderDesc& SetFilename(const std::string& value) { filename = value; return *this; }
	ShaderDesc& SetEntry(const std::string& value) { entry = value; return *this; }
	constexpr ShaderDesc& SetShaderType(ShaderType value) noexcept { type = value; return *this; }
};

} // namespace Luna