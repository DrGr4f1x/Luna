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

#include "Shader.h"

#include "BinaryReader.h"
#include "FileSystem.h"

using namespace std;


namespace Luna
{

map<size_t, unique_ptr<Shader>> s_shaderHashMap;
mutex s_shaderMapMutex;


pair<Shader*, bool> FindOrLoadShader(const ShaderDesc& shaderDesc)
{
	size_t hashCode = hash<string>{}(shaderDesc.filenameWithExtension);

	lock_guard<mutex> CS(s_shaderMapMutex);

	auto iter = s_shaderHashMap.find(hashCode);

	// If it's found, it has already been loaded or the load process has begun
	if (iter != s_shaderHashMap.end())
	{
		return make_pair(iter->second.get(), false);
	}

	Shader* shader = new Shader(shaderDesc);
	s_shaderHashMap[hashCode].reset(shader);

	// This was the first time it was requested, so indicate that the caller must read the file
	return make_pair(shader, true);
}


void Shader::DestroyAll()
{
	lock_guard<mutex> CS(s_shaderMapMutex);
	s_shaderHashMap.clear();
}


Shader* Shader::Load(const ShaderDesc& shaderDesc)
{
	auto [shader, requestsLoad] = FindOrLoadShader(shaderDesc);

	// Wait on the load and return
	if (!requestsLoad)
	{
		shader->WaitForLoad();
		return shader;
	}

	// Kick off the load
	auto fileSystem = GetFileSystem();
	string fullpath = fileSystem->GetFullPath(shader->GetFilenameWithExtension());

	assert_succeeded(BinaryReader::ReadEntireFile(fullpath, shader->m_byteCode, &shader->m_byteCodeSize));
	shader->m_isLoaded = true;

	return shader;
}


Shader::Shader(const ShaderDesc& shaderDesc)
	: m_filenameWithExtension{ shaderDesc.filenameWithExtension }
	, m_entry{ shaderDesc.entry }
	, m_type{ shaderDesc.type }
{}


void Shader::WaitForLoad() const
{
	volatile bool& volIsLoaded = (volatile bool&)m_isLoaded;
	while (!volIsLoaded)
	{
		this_thread::yield();
	}
}

} // namespace Luna