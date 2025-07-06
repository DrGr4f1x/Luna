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

#include "Graphics\GraphicsCommon.h"
#include "Graphics\PixelBuffer.h"

namespace Luna
{

// Forward declarations
class IDevice;
class TextureManager;
class TexturePtr;


struct TextureSubresourceData
{
	// For D3D12_SUBRESOURCE_DATA
	std::byte* data{ nullptr };
	uint64_t rowPitch{ 0 };
	uint64_t slicePitch{ 0 };

	// For VkBufferImageCopy
	size_t bufferOffset{ 0 };
	uint32_t mipLevel{ 0 };
	uint32_t baseArrayLayer{ 0 };
	uint32_t layerCount{ 0 };
	uint32_t width{ 0 };
	uint32_t height{ 0 };
	uint32_t depth{ 0 };
};


struct TextureInitializer
{
	Format format{ Format::Unknown };
	TextureDimension dimension{ TextureDimension::Unknown };
	uint64_t width{ 0 };
	uint32_t height{ 0 };
	uint32_t arraySizeOrDepth{ 1 };
	uint32_t numMips{ 1 };
	std::vector<TextureSubresourceData> subResourceData;

	std::byte* baseData{ nullptr };
	size_t totalBytes{ 0 };

	uint32_t GetSubresourceIndex(GraphicsApi api, uint32_t arraySlice, uint32_t face, uint32_t mipLevel);
};


struct TextureDesc
{
	std::string name;
	uint64_t width{ 0 };
	uint32_t height{ 0 };
	uint32_t depth{ 0 };
	Format format{ Format::Unknown };
	std::byte* data{ nullptr };
};


class ITexture : public IPixelBuffer
{
	friend class TextureManager;
	friend class TexturePtr;

public:
	virtual bool IsValid() const = 0;
	bool IsLoading() const { return m_isLoading; }
	void WaitForLoad() const;

protected:
	virtual unsigned long AddRef();
	virtual unsigned long Release();

protected:
	std::string m_mapKey;
	std::atomic_ulong m_refCount{ 0 };
	std::atomic<bool> m_isLoading{ true };
	bool m_isManaged{ false };
};


class TexturePtr
{
	friend class TextureManager;

public:
	TexturePtr(const TexturePtr& ptr);
	TexturePtr(ITexture* tex = nullptr);
	~TexturePtr();

	void operator=(std::nullptr_t);
	void operator=(const TexturePtr& rhs);

	bool IsValid() const;

	ITexture* Get();
	ITexture* operator->();

	const ITexture* Get() const;
	const ITexture* operator->() const;

private:
	ITexture* m_tex{ nullptr };
};


class TextureManager
{
public:
	explicit TextureManager(IDevice* device);
	~TextureManager();

	TexturePtr Load(const std::string& filename, bool forceSrgb = false);
	void DestroyTexture(const std::string& key);

protected:
	TexturePtr FindOrLoadTexture(const std::string& filename, bool forceSrgb);
	bool LoadTextureFromFile(ITexture* tex, const std::string& filename, bool forceSrgb);

protected:
	IDevice* m_device{ nullptr };

	std::mutex m_mutex;
	std::unordered_map<std::string, std::unique_ptr<ITexture>> m_textureMap;
};


TextureManager* GetTextureManager();

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
	TextureInitializer& outTexInit);

} // namespace Luna