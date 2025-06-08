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

#include "Graphics\Vulkan\EnumsVK.h"
#include "Graphics\Vulkan\RefCountingImplVK.h"


#ifdef CreateSemaphore
#undef CreateSemaphore
#endif

namespace Luna::VK
{

struct ImageDesc
{
	std::string name;
	uint64_t width{ 0 };
	uint32_t height{ 0 };
	uint32_t arraySizeOrDepth{ 1 };
	Format format{ Format::Unknown };
	uint32_t numMips{ 1 };
	uint32_t numSamples{ 1 };
	ResourceType resourceType{ ResourceType::Unknown };
	GpuImageUsage imageUsage{ GpuImageUsage::Unknown };
	MemoryAccess memoryAccess{ MemoryAccess::Unknown };

	ImageDesc& SetName(const std::string& value) { name = value; return *this; }
	constexpr ImageDesc& SetWidth(uint64_t value) noexcept { width = value; return *this; }
	constexpr ImageDesc& SetHeight(uint32_t value) noexcept { height = value; return *this; }
	constexpr ImageDesc& SetArraySize(uint32_t value) noexcept { arraySizeOrDepth = value; return *this; }
	constexpr ImageDesc& SetDepth(uint32_t value) noexcept { arraySizeOrDepth = value; return *this; }
	constexpr ImageDesc& SetFormat(Format value) noexcept { format = value; return *this; }
	constexpr ImageDesc& SetNumMips(uint32_t value) noexcept { numMips = value; return *this; }
	constexpr ImageDesc& SetNumSamples(uint32_t value) noexcept { numSamples = value; return *this; }
	constexpr ImageDesc& SetResourceType(ResourceType value) noexcept { resourceType = value; return *this; }
	constexpr ImageDesc& SetImageUsage(GpuImageUsage value) noexcept { imageUsage = value; return *this; }
	constexpr ImageDesc& SetMemoryAccess(MemoryAccess value) noexcept { memoryAccess = value; return *this; }
};

wil::com_ptr<CVkImage> CreateImage(CVkDevice* device, CVmaAllocator* allocator, const ImageDesc& desc);


struct ImageViewDesc
{
	CVkImage* image{ nullptr };
	std::string name;
	ResourceType resourceType{ ResourceType::Unknown };
	GpuImageUsage imageUsage{ GpuImageUsage::Unknown };
	Format format{ Format::Unknown };
	ImageAspect imageAspect{ 0 };
	TextureSubresourceViewType viewType{ TextureSubresourceViewType::AllAspects };
	uint32_t baseMipLevel{ 0 };
	uint32_t mipCount{ 1 };
	uint32_t baseArraySlice{ 0 };
	uint32_t arraySize{ 1 };

	constexpr ImageViewDesc& SetImage(CVkImage* value) noexcept { image = value; return *this; }
	ImageViewDesc& SetName(const std::string& value) { name = value; return *this; }
	constexpr ImageViewDesc& SetResourceType(ResourceType value) noexcept { resourceType = value; return *this; }
	constexpr ImageViewDesc& SetImageUsage(GpuImageUsage value) noexcept { imageUsage = value; return *this; }
	constexpr ImageViewDesc& SetFormat(Format value) noexcept { format = value; return *this; }
	constexpr ImageViewDesc& SetImageAspect(ImageAspect value) noexcept { imageAspect = value; return *this; }
	constexpr ImageViewDesc& SetViewType(TextureSubresourceViewType value) noexcept { viewType = value; return *this; }
	constexpr ImageViewDesc& SetBaseMipLevel(uint32_t value) noexcept { baseMipLevel = value; return *this; }
	constexpr ImageViewDesc& SetMipCount(uint32_t value) noexcept { mipCount = value; return *this; }
	constexpr ImageViewDesc& SetBaseArraySlice(uint32_t value) noexcept { baseArraySlice = value; return *this; }
	constexpr ImageViewDesc& SetArraySize(uint32_t value) noexcept { arraySize = value; return *this; }
};


struct ImageData
{
	wil::com_ptr<CVkImage> image{ nullptr };
	ResourceState usageState{ ResourceState::Undefined };
};


struct BufferData
{
	wil::com_ptr<CVkBuffer> buffer{ nullptr };
	ResourceState usageState{ ResourceState::Undefined };
};


wil::com_ptr<CVkImageView> CreateImageView(CVkDevice* device, const ImageViewDesc& desc);

wil::com_ptr<CVkFence> CreateFence(CVkDevice* device, bool bSignalled);
wil::com_ptr<CVkSemaphore> CreateSemaphore(CVkDevice* device, VkSemaphoreType semaphoreType, uint64_t initialValue);
wil::com_ptr<CVkBuffer> CreateStagingBuffer(CVkDevice* device, CVmaAllocator* allocator, const void* initialData, size_t numBytes);

}