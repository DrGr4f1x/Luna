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

#include "Shader12.h"

#include "FileSystem.h"

using namespace std;


namespace Luna::DX12
{

pair<string, bool> GetShaderFilenameWithExtension(const string& shaderFilename)
{
	auto fileSystem = GetFileSystem();

	string shaderFileWithExtension = shaderFilename;
	bool exists = false;

	// See if the filename already has an extension
	string extension = fileSystem->GetFileExtension(shaderFilename);
	if (!extension.empty())
	{
		exists = fileSystem->Exists(shaderFileWithExtension);
	}
	else
	{
		// Try .dxil extension first
		shaderFileWithExtension = shaderFilename + ".dxil";
		exists = fileSystem->Exists(shaderFileWithExtension);
		if (!exists)
		{
			// Try .dxbc next
			shaderFileWithExtension = shaderFilename + ".dxbc";
			exists = fileSystem->Exists(shaderFileWithExtension);
		}
	}

	return make_pair(shaderFileWithExtension, exists);
}


Shader* LoadShader(ShaderType type, const ShaderNameAndEntry& shaderNameAndEntry)
{
	auto [shaderFilenameWithExtension, exists] = GetShaderFilenameWithExtension(shaderNameAndEntry.shaderFile);

	if (!exists)
	{
		return nullptr;
	}

	ShaderDesc shaderDesc{
		.filenameWithExtension	= shaderFilenameWithExtension,
		.entry					= shaderNameAndEntry.entry,
		.type					= type
	};

	return Shader::Load(shaderDesc);
}

} // namespace Luna::DX12