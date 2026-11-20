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

#include "DirectXCommon.h"

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
	std::string shaderFile = shaderNameAndEntry.shaderFile;
#if 0
	if (shaderNameAndEntry.entry != "main")
	{
		shaderFile = std::format("{}_{}", shaderNameAndEntry.shaderFile, shaderNameAndEntry.entry);
	}
#endif

	auto [shaderFilenameWithExtension, exists] = GetShaderFilenameWithExtension(shaderFile);

	if (!exists)
	{
		return nullptr;
	}

	ShaderDesc shaderDesc{
		.filenameWithExtension	= shaderFilenameWithExtension,
		.entry					= shaderNameAndEntry.entry,
		.type					= type
	};

	auto* shader = Shader::Load(shaderDesc);

#if 0
	DxcBuffer dxcBuffer{
		.Ptr = shader->GetByteCode(),
		.Size = shader->GetByteCodeSize(),
		.Encoding = DXC_CP_ACP
	};

	wil::com_ptr<IDxcUtils> dxcUtils;
	DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));

	wil::com_ptr<ID3D12ShaderReflection> reflection;
	HRESULT hr = dxcUtils->CreateReflection(&dxcBuffer, IID_PPV_ARGS(&reflection));
	if (SUCCEEDED(hr))
	{
		D3D12_SHADER_DESC shaderDesc12{};
		reflection->GetDesc(&shaderDesc12);
	}
	else
	{
		wil::com_ptr<ID3D12LibraryReflection> libraryReflection;
		hr = dxcUtils->CreateReflection(&dxcBuffer, IID_PPV_ARGS(&libraryReflection));
		if (SUCCEEDED(hr))
		{
			D3D12_LIBRARY_DESC libraryDesc12{};
			libraryReflection->GetDesc(&libraryDesc12);
			for (uint32_t i = 0; i < libraryDesc12.FunctionCount; ++i)
			{
				auto functionReflection = libraryReflection->GetFunctionByIndex(i);
				D3D12_FUNCTION_DESC functionDesc12{};
				functionReflection->GetDesc(&functionDesc12); 
				__debugbreak();

			}
			__debugbreak();
		}
		else
		{
			__debugbreak();
		}
	}
#endif

	return shader;
}

} // namespace Luna::DX12