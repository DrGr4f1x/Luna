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

#include "Texture.h"

#include "BinaryReader.h"
#include "FileSystem.h"

#include "Graphics\Device.h"
#include "Graphics\Loaders\DDSTextureLoader.h"
#include "Graphics\Loaders\KTXTextureLoader.h"
#include "Graphics\Loaders\STBTextureLoader.h"


namespace Luna
{

static TextureManager* g_textureManager{ nullptr };


void ITexture::WaitForLoad() const
{
	while (m_isLoading)
	{
		std::this_thread::yield();
	}
}

unsigned long ITexture::AddRef()
{
	return ++m_refCount;
}


unsigned long ITexture::Release()
{
	unsigned long result = --m_refCount;
	if (result == 0)
	{
		if (m_isManaged)
		{
			auto textureManager = GetTextureManager();
			textureManager->DestroyTexture(m_mapKey);
		}
		else
		{
			delete this;
		}
	}
	return result;
}


TexturePtr::TexturePtr(const TexturePtr& ptr) 
	: m_tex{ ptr.m_tex }
{
	if (m_tex != nullptr)
	{
		m_tex->AddRef();
	}
}


TexturePtr::TexturePtr(ITexture* tex) 
	: m_tex{ tex }
{
	if (m_tex != nullptr)
	{
		m_tex->AddRef();
	}
}


TexturePtr::~TexturePtr()
{
	if (m_tex != nullptr)
	{
		m_tex->Release();
	}
}


void TexturePtr::operator=(std::nullptr_t)
{
	if (m_tex != nullptr)
	{
		m_tex->Release();
	}

	m_tex = nullptr;
}


void TexturePtr::operator=(const TexturePtr& rhs)
{
	if (m_tex != nullptr)
	{
		m_tex->Release();
	}

	m_tex = rhs.m_tex;

	if (m_tex != nullptr)
	{
		m_tex->AddRef();
	}		
}


bool TexturePtr::IsValid() const
{
	return m_tex && m_tex->IsValid();
}


ITexture* TexturePtr::Get()
{
	return m_tex;
}


ITexture* TexturePtr::operator->()
{
	assert(m_tex != nullptr);
	return m_tex;
}

const ITexture* TexturePtr::Get() const
{
	return m_tex;
}


const ITexture* TexturePtr::operator->() const
{
	assert(m_tex != nullptr);
	return m_tex;
}


TextureManager::TextureManager(IDevice* device)
	: m_device{ device }
{
	assert(g_textureManager == nullptr);
	g_textureManager = this;
}


TextureManager::~TextureManager()
{
	g_textureManager = nullptr;
}


TexturePtr TextureManager::Load(const std::string& filename, bool forceSrgb)
{
	return FindOrLoadTexture(filename, forceSrgb);
}


void TextureManager::DestroyTexture(const std::string& key)
{
	std::lock_guard lock(m_mutex);

	auto iter = m_textureMap.find(key);
	if (iter != m_textureMap.end())
	{
		m_textureMap.erase(iter);
	}
}


TexturePtr TextureManager::FindOrLoadTexture(const std::string& filename, bool forceSrgb)
{
	TexturePtr tex;

	{
		std::lock_guard lock(m_mutex);

		std::string key = filename;
		if (forceSrgb)
		{
			key += "_SRGB";
		}

		auto iter = m_textureMap.find(key);

		if (iter != m_textureMap.end())
		{
			tex = iter->second.get();
			tex->WaitForLoad();
			return tex;
		}
		else
		{
			tex = m_device->CreateUninitializedTexture(filename, key);
			m_textureMap[key].reset(tex.Get());
		}
	}

	LoadTextureFromFile(tex.Get(), filename, forceSrgb);
	return tex;
}


bool TextureManager::LoadTextureFromFile(ITexture* tex, const std::string& filename, bool forceSrgb)
{
	auto fileSystem = GetFileSystem();

	bool loadSucceeded = false;

	if (fileSystem->Exists(filename))
	{
		std::string extension = fileSystem->GetFileExtension(filename);

		size_t dataSize{ 0 };
		std::unique_ptr<std::byte[]> data;
		BinaryReader::ReadEntireFile(fileSystem->GetFullPath(filename), data, &dataSize);

		if (extension == ".dds")
		{
			CreateDDSTextureFromMemory(m_device, tex, filename, data.get(), dataSize, Format::Unknown, forceSrgb);
		}
		else if (extension == ".ktx")
		{

		}
		else
		{

		}

		loadSucceeded = tex->IsValid();
	}

	tex->m_isManaged = true;
	tex->m_isLoading = false;
	return loadSucceeded;
}


TextureManager* GetTextureManager()
{
	assert(g_textureManager != nullptr);
	return g_textureManager;
}

} // namespace Luna