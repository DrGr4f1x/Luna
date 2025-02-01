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
	std::string filenameWithExtension;
	std::string entry{ "main" };
	ShaderType type{ ShaderType::None };

	ShaderDesc& SetFilenameWithExtension(const std::string& value) { filenameWithExtension = value; return *this; }
	ShaderDesc& SetEntry(const std::string& value) { entry = value; return *this; }
	constexpr ShaderDesc& SetShaderType(ShaderType value) noexcept { type = value; return *this; }
};


class Shader
{
public:
	static void DestroyAll();
	static Shader* Load(const ShaderDesc& shaderDesc);

	explicit Shader(const ShaderDesc& shaderDesc);
	
	const std::string& GetFilenameWithExtension() const { return m_filenameWithExtension; }
	const std::string& GetEntry() const { return m_entry; }
	ShaderType GetShaderType() const { return m_type; }

	const std::byte* GetByteCode() const { return m_byteCode.get(); }
	size_t GetByteCodeSize() const { return m_byteCodeSize; }

	size_t GetHash() const { return m_hash; }

private:
	void WaitForLoad() const;

private:
	std::string m_filenameWithExtension;
	std::string m_entry{ "main" };
	ShaderType m_type{ ShaderType::None };

	std::unique_ptr<std::byte[]> m_byteCode;
	size_t m_byteCodeSize{ 0 };
	size_t m_hash{ 0 };

	bool m_isLoaded{ false };
};

} // namespace Luna