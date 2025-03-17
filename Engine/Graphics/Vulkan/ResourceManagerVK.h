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

#include "Graphics\ColorBuffer.h"
#include "Graphics\DepthBuffer.h"
#include "Graphics\GpuBuffer.h"
#include "Graphics\ResourceManager.h"
#include "Graphics\Vulkan\VulkanCommon.h"


namespace Luna::VK
{

struct ResourceData
{
	wil::com_ptr<CVkImage> image{ nullptr };
	wil::com_ptr<CVkBuffer> buffer{ nullptr };
	// TODO: Use Vulkan type directly
	ResourceState usageState{ ResourceState::Undefined };
};


struct ColorBufferData
{
	wil::com_ptr<CVkImageView> imageViewRtv{ nullptr };
	wil::com_ptr<CVkImageView> imageViewSrv{ nullptr };
	VkDescriptorImageInfo imageInfoSrv{};
	VkDescriptorImageInfo imageInfoUav{};
	uint32_t planeCount{ 1 };
};


struct DepthBufferData
{
	wil::com_ptr<CVkImageView> imageViewDepthStencil;
	wil::com_ptr<CVkImageView> imageViewDepthOnly;
	wil::com_ptr<CVkImageView> imageViewStencilOnly;
	VkDescriptorImageInfo imageInfoDepth{};
	VkDescriptorImageInfo imageInfoStencil{};
	uint32_t planeCount{ 1 };
};


struct GpuBufferData
{
	wil::com_ptr<CVkBufferView> bufferView;
	VkDescriptorBufferInfo bufferInfo;
};


class ResourceManager : public IResourceManager
{
	static const uint32_t MaxResources = (1 << 12);
	static const uint32_t InvalidAllocation = ~0u;
	enum ManagedResourceType
	{
		ManagedColorBuffer = 0x1,
		ManagedDepthBuffer = 0x2,
		ManagedGpuBuffer = 0x4,
	};

public:
	ResourceManager(CVkDevice* device, CVmaAllocator* allocator);
	~ResourceManager();

	// Creation/destruction methods
	ResourceHandle CreateColorBuffer(const ColorBufferDesc& colorBufferDesc) override;
	ResourceHandle CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc) override;
	ResourceHandle CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc) override;
	void DestroyHandle(ResourceHandleType* handle) override;

	// General resource methods
	std::optional<ResourceType> GetResourceType(ResourceHandleType* handle) const override;
	std::optional<ResourceState> GetUsageState(ResourceHandleType* handle) const override;
	void SetUsageState(ResourceHandleType* handle, ResourceState newState) override;
	std::optional<Format> GetFormat(ResourceHandleType* handle) const override;

	// Pixel buffer methods
	std::optional<uint64_t> GetWidth(ResourceHandleType* handle) const override;
	std::optional<uint32_t> GetHeight(ResourceHandleType* handle) const override;
	std::optional<uint32_t> GetDepthOrArraySize(ResourceHandleType* handle) const override;
	std::optional<uint32_t> GetNumMips(ResourceHandleType* handle) const override;
	std::optional<uint32_t> GetNumSamples(ResourceHandleType* handle) const override;
	std::optional<uint32_t> GetPlaneCount(ResourceHandleType* handle) const override;

	// Color buffer methods
	std::optional<Color> GetClearColor(ResourceHandleType* handle) const override;

	// Depth buffer methods
	std::optional<float> GetClearDepth(ResourceHandleType* handle) const override;
	std::optional<uint8_t> GetClearStencil(ResourceHandleType* handle) const override;

	// Gpu buffer methods
	std::optional<size_t> GetSize(ResourceHandleType* handle) const override;
	std::optional<size_t> GetElementCount(ResourceHandleType* handle) const override;
	std::optional<size_t> GetElementSize(ResourceHandleType* handle) const override;
	void Update(ResourceHandleType* handle, size_t sizeInBytes, size_t offset, const void* data) const override;

	// Platform specific methods
	ResourceHandle CreateColorBufferFromSwapChainImage(CVkImage* swapChainImage, uint32_t width, uint32_t height, Format format, uint32_t imageIndex);
	VkImage GetImage(ResourceHandleType* handle) const;
	VkBuffer GetBuffer(ResourceHandleType* handle) const;
	VkImageView GetImageViewSrv(ResourceHandleType* handle) const;
	VkImageView GetImageViewRtv(ResourceHandleType* handle) const;
	VkImageView GetImageViewDepth(ResourceHandleType* handle, DepthStencilAspect depthStencilAspect) const;
	VkDescriptorImageInfo GetImageInfoSrv(ResourceHandleType* handle) const;
	VkDescriptorImageInfo GetImageInfoUav(ResourceHandleType* handle) const;
	VkDescriptorImageInfo GetImageInfoDepth(ResourceHandleType* handle, bool depthSrv) const;
	VkDescriptorBufferInfo GetBufferInfo(ResourceHandleType* handle) const;
	VkBufferView GetBufferView(ResourceHandleType* handle) const;

private:
	std::tuple<uint32_t, uint32_t, uint32_t> UnpackHandle(ResourceHandleType* handle) const;
	std::pair<ResourceData, ColorBufferData> CreateColorBuffer_Internal(const ColorBufferDesc& colorBufferDesc);
	std::pair<ResourceData, DepthBufferData> CreateDepthBuffer_Internal(const DepthBufferDesc& depthBufferDesc);
	std::pair<ResourceData, GpuBufferData> CreateGpuBuffer_Internal(const GpuBufferDesc& gpuBufferDesc);

private:
	wil::com_ptr<CVkDevice> m_device;
	wil::com_ptr<CVmaAllocator> m_allocator;

	// Allocation mutex
	std::mutex m_allocationMutex;

	// Resource freelist
	std::queue<uint32_t> m_resourceFreeList;

	// Resource indices
	std::array<uint32_t, MaxResources> m_resourceIndices;

	// Resource data
	std::array<ResourceData, MaxResources> m_resourceData;

	// Data caches
	template<typename DescType, typename DataType, uint32_t MAX_ITEMS>
	struct TDataCache
	{
		std::queue<uint32_t> freeList;
		std::array<DescType, MAX_ITEMS> descArray;
		std::array<DataType, MAX_ITEMS> dataArray;

		void AddData(uint32_t dataIndex, const DescType& desc, const DataType& data)
		{
			descArray[dataIndex] = desc;
			dataArray[dataIndex] = data;
		}

		void Reset(uint32_t dataIndex)
		{
			freeList.push(dataIndex);
			descArray[dataIndex] = DescType{};
			dataArray[dataIndex] = DataType{};
		}
	};

	TDataCache<ColorBufferDesc, ColorBufferData, MaxResources> m_colorBufferCache;
	TDataCache<DepthBufferDesc, DepthBufferData, MaxResources> m_depthBufferCache;
	TDataCache<GpuBufferDesc, GpuBufferData, MaxResources> m_gpuBufferCache;
};


ResourceManager* const GetVulkanResourceManager();

} // namespace Luna::VK