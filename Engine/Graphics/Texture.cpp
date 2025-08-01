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


uint32_t TextureInitializer::GetSubresourceIndex(GraphicsApi api, uint32_t face, uint32_t arraySlice, uint32_t mipLevel)
{
	const bool isCubemap = dimension == TextureDimension::TextureCube || dimension == TextureDimension::TextureCube_Array;
	const uint32_t numFaces = isCubemap ? 6 : 1;
	const uint32_t effectiveArraySize = isCubemap ? arraySizeOrDepth / 6 : arraySizeOrDepth;

	assert(arraySlice < arraySizeOrDepth);
	assert(face < numFaces);
	assert(mipLevel < numMips);

	if (api == GraphicsApi::Vulkan)
	{
		return (face * effectiveArraySize * numMips) + (arraySlice * numMips) + mipLevel;
	}
	else
	{
		return (arraySlice * numFaces * numMips) + (face * numMips) + mipLevel;
	}
}


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


void ITexture::SetData(std::byte* data, size_t dataSize)
{
	m_dataSize = dataSize;
	m_data.reset(new std::byte[dataSize]);
	memcpy(m_data.get(), data, dataSize);
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

ITexture* TexturePtr::Get() const
{
	return m_tex;
}


ITexture* TexturePtr::operator->() const
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


TexturePtr TextureManager::Load(const std::string& filename, Format format, bool forceSrgb, bool retainData)
{
	return FindOrLoadTexture(filename, format, forceSrgb, retainData);
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


TexturePtr TextureManager::FindOrLoadTexture(const std::string& filename, Format format, bool forceSrgb, bool retainData)
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

	LoadTextureFromFile(tex.Get(), filename, format, forceSrgb, retainData);
	return tex;
}


bool TextureManager::LoadTextureFromFile(ITexture* tex, const std::string& filename, Format format, bool forceSrgb, bool retainData)
{
	auto fileSystem = GetFileSystem();

	bool loadSucceeded = false;

	if (fileSystem->Exists(filename))
	{
		std::string extension = fileSystem->GetFileExtension(filename);

		size_t dataSize{ 0 };
		std::unique_ptr<std::byte[]> data;
		BinaryReader::ReadEntireFile(fileSystem->GetFullPath(filename), data, &dataSize);

		CreateTextureFromMemory(m_device, tex, filename, data.get(), dataSize, format, forceSrgb, retainData);

		loadSucceeded = tex->IsValid();
	}

	tex->m_isManaged = true;
	tex->m_isLoading = false;
	return loadSucceeded;
}


bool CreateTextureFromMemory(IDevice* device, ITexture* texture, const std::string& textureName, std::byte* data, size_t dataSize, Format format, bool forceSrgb, bool retainData)
{
	auto fileSystem = GetFileSystem();

	std::string extension = fileSystem->GetFileExtension(textureName);

	if (extension == ".dds")
	{
		return CreateDDSTextureFromMemory(device, texture, textureName, data, dataSize, format, forceSrgb, retainData);
	}
	else if (extension == ".ktx" || extension == ".ktx2")
	{
		return CreateKTXTextureFromMemory(device, texture, textureName, data, dataSize, format, forceSrgb, retainData);
	}
	else
	{
		return CreateSTBTextureFromMemory(device, texture, textureName, data, dataSize, format, forceSrgb, retainData);
	}
}


TextureManager* GetTextureManager()
{
	assert(g_textureManager != nullptr);
	return g_textureManager;
}


bool FillTextureInitializer(
	size_t width,
	size_t height,
	size_t depth,
	size_t mipCount,
	size_t arraySize,
	Format format,
	size_t maxSize,
	size_t bitSize,
	std::byte* bitData,
	size_t& skipMip,
	TextureInitializer& outTexInit)
{
	if (!bitData || outTexInit.subResourceData.empty())
	{
		return false;
	}

	skipMip = 0;
	//outTexInit.width = 0;
	//outTexInit.height = 0;
	//outTexInit.arraySizeOrDepth = 0;
	outTexInit.baseData = bitData;
	outTexInit.totalBytes = bitSize;

	size_t numBytes = 0;
	size_t rowBytes = 0;
	size_t offset = 0;
	std::byte* pSrcBits = bitData;
	std::byte* pEndBits = bitData + bitSize;

	size_t index = 0;
	for (size_t j = 0; j < arraySize; j++)
	{
		size_t w = width;
		size_t h = height;
		size_t d = depth;
		for (size_t i = 0; i < mipCount; i++)
		{
			GetSurfaceInfo(w, h, format, &numBytes, &rowBytes, nullptr, nullptr, nullptr);

			if ((mipCount <= 1) || !maxSize || (w <= maxSize && h <= maxSize && d <= maxSize))
			{
				/*if (!outTexInit.width)
				{
					outTexInit.width = w;
					outTexInit.height = (uint32_t)h;
					outTexInit.arraySizeOrDepth = (uint32_t)d;
				}*/

				assert(index < mipCount * arraySize);
				_Analysis_assume_(index < mipCount * arraySize);

				// Fill in data for DX12
				auto& subResourceData = outTexInit.subResourceData[index];
				subResourceData.data = pSrcBits;
				subResourceData.rowPitch = (uint32_t)rowBytes;
				subResourceData.slicePitch = (uint32_t)numBytes;

				// Fill in data for Vulkan
				subResourceData.bufferOffset = offset;
				subResourceData.mipLevel = (uint32_t)i;
				subResourceData.baseArrayLayer = (uint32_t)j;
				subResourceData.layerCount = 1;
				subResourceData.width = (uint32_t)w;
				subResourceData.height = (uint32_t)h;
				subResourceData.depth = (uint32_t)d;

				++index;
			}
			else if (!j)
			{
				// Count number of skipped mipmaps (first item only)
				++skipMip;
			}

			if (pSrcBits + (numBytes * d) > pEndBits)
			{
				return false;
			}

			pSrcBits += numBytes * d;
			offset += (uint32_t)numBytes;

			w = w >> 1;
			h = h >> 1;
			d = d >> 1;

			w = std::max<size_t>(w, 1);
			h = std::max<size_t>(h, 1);
			d = std::max<size_t>(d, 1);
		}
	}

	return (index > 0);
}

} // namespace Luna