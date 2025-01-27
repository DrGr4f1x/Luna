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
#include "Graphics\PlatformData.h"


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


class __declspec(uuid("19593A92-B396-4411-ADB6-73DCEA832CE6")) IShaderData : public IPlatformData
{
public:
	virtual const std::byte* GetByteCode() const = 0;
	virtual size_t GetByteCodeSize() const = 0;
};


class Shader
{
public:
	bool Initialize(const ShaderDesc& desc);

	const std::byte* GetByteCode() const;
	size_t GetByteCodeSize() const;

	IShaderData* GetShaderData() const { return m_shaderData.get(); }

private:
	wil::com_ptr<IShaderData> m_shaderData;
};

} // namespace Luna