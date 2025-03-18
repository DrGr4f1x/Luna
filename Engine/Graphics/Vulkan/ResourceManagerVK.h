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
#include "Graphics\PipelineState.h"
#include "Graphics\ResourceManager.h"
#include "Graphics\Shader.h"
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


struct DescriptorBindingDesc
{
	VkDescriptorType descriptorType;
	uint32_t startSlot{ 0 };
	uint32_t numDescriptors{ 1 };
	uint32_t offset{ 0 };
};


struct RootSignatureData
{
	wil::com_ptr<CVkPipelineLayout> pipelineLayout;
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
	std::unordered_map<uint32_t, std::vector<DescriptorBindingDesc>> layoutBindingMap;
	std::unordered_map<uint32_t, uint32_t> rootParameterIndexToDescriptorSetMap;
	std::vector<wil::com_ptr<CVkDescriptorSetLayout>> descriptorSetLayouts;
};


class ResourceManager : public IResourceManager
{
	static const uint32_t MaxResources = (1 << 12);
	static const uint32_t InvalidAllocation = ~0u;
	enum ManagedResourceType
	{
		ManagedColorBuffer			= 0x0001,
		ManagedDepthBuffer			= 0x0002,
		ManagedGpuBuffer			= 0x0004,
		ManagedGraphicsPipeline		= 0x0008,
		ManagedRootSignature		= 0x0010
	};

public:
	ResourceManager(CVkDevice* device, CVmaAllocator* allocator);
	~ResourceManager();

	// Creation/destruction methods
	ResourceHandle CreateColorBuffer(const ColorBufferDesc& colorBufferDesc) override;
	ResourceHandle CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc) override;
	ResourceHandle CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc) override;
	ResourceHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& pipelineDesc) override;
	ResourceHandle CreateRootSignature(const RootSignatureDesc& rootSignatureDesc) override;
	void DestroyHandle(const ResourceHandleType* handle) override;

	// General resource methods
	std::optional<ResourceType> GetResourceType(const ResourceHandleType* handle) const override;
	std::optional<ResourceState> GetUsageState(const ResourceHandleType* handle) const override;
	void SetUsageState(const ResourceHandleType* handle, ResourceState newState) override;
	std::optional<Format> GetFormat(const ResourceHandleType* handle) const override;

	// Pixel buffer methods
	std::optional<uint64_t> GetWidth(const ResourceHandleType* handle) const override;
	std::optional<uint32_t> GetHeight(const ResourceHandleType* handle) const override;
	std::optional<uint32_t> GetDepthOrArraySize(const ResourceHandleType* handle) const override;
	std::optional<uint32_t> GetNumMips(const ResourceHandleType* handle) const override;
	std::optional<uint32_t> GetNumSamples(const ResourceHandleType* handle) const override;
	std::optional<uint32_t> GetPlaneCount(const ResourceHandleType* handle) const override;

	// Color buffer methods
	std::optional<Color> GetClearColor(const ResourceHandleType* handle) const override;

	// Depth buffer methods
	std::optional<float> GetClearDepth(const ResourceHandleType* handle) const override;
	std::optional<uint8_t> GetClearStencil(const ResourceHandleType* handle) const override;

	// Gpu buffer methods
	std::optional<size_t> GetSize(const ResourceHandleType* handle) const override;
	std::optional<size_t> GetElementCount(const ResourceHandleType* handle) const override;
	std::optional<size_t> GetElementSize(const ResourceHandleType* handle) const override;
	void Update(const ResourceHandleType* handle, size_t sizeInBytes, size_t offset, const void* data) const override;

	// Graphics pipeline state
	const GraphicsPipelineDesc& GetGraphicsPipelineDesc(const ResourceHandleType* handle) const override;

	// Root signature methods
	const RootSignatureDesc& GetRootSignatureDesc(const ResourceHandleType* handle) const override;
	uint32_t GetNumRootParameters(const ResourceHandleType* handle) const override;
	wil::com_ptr<DescriptorSetHandleType> CreateDescriptorSet(const ResourceHandleType* handle, uint32_t rootParamIndex) const override;

	// Platform specific methods
	ResourceHandle CreateColorBufferFromSwapChainImage(CVkImage* swapChainImage, uint32_t width, uint32_t height, Format format, uint32_t imageIndex);
	VkImage GetImage(const ResourceHandleType* handle) const;
	VkBuffer GetBuffer(const ResourceHandleType* handle) const;
	VkImageView GetImageViewSrv(const ResourceHandleType* handle) const;
	VkImageView GetImageViewRtv(const ResourceHandleType* handle) const;
	VkImageView GetImageViewDepth(const ResourceHandleType* handle, DepthStencilAspect depthStencilAspect) const;
	VkDescriptorImageInfo GetImageInfoSrv(const ResourceHandleType* handle) const;
	VkDescriptorImageInfo GetImageInfoUav(const ResourceHandleType* handle) const;
	VkDescriptorImageInfo GetImageInfoDepth(const ResourceHandleType* handle, bool depthSrv) const;
	VkDescriptorBufferInfo GetBufferInfo(const ResourceHandleType* handle) const;
	VkBufferView GetBufferView(const ResourceHandleType* handle) const;
	VkPipeline GetGraphicsPipeline(const ResourceHandleType* handle) const;
	VkPipelineLayout GetPipelineLayout(const ResourceHandleType* handle) const;
	CVkDescriptorSetLayout* GetDescriptorSetLayout(const ResourceHandleType* handle, uint32_t paramIndex) const;
	int GetDescriptorSetIndexFromRootParameterIndex(const ResourceHandleType* handle, uint32_t paramIndex) const;
	const std::vector<DescriptorBindingDesc>& GetLayoutBindings(const ResourceHandleType* handle, uint32_t paramIndex) const;

private:
	std::tuple<uint32_t, uint32_t, uint32_t> UnpackHandle(const ResourceHandleType* handle) const;
	std::pair<ResourceData, ColorBufferData> CreateColorBuffer_Internal(const ColorBufferDesc& colorBufferDesc);
	std::pair<ResourceData, DepthBufferData> CreateDepthBuffer_Internal(const DepthBufferDesc& depthBufferDesc);
	std::pair<ResourceData, GpuBufferData> CreateGpuBuffer_Internal(const GpuBufferDesc& gpuBufferDesc);
	wil::com_ptr<CVkPipeline> CreateGraphicsPipeline_Internal(const GraphicsPipelineDesc& pipelineDesc);
	RootSignatureData CreateRootSignature_Internal(const RootSignatureDesc& rootSignatureDesc);
	wil::com_ptr<CVkShaderModule> CreateShaderModule(Shader* shader);
	wil::com_ptr<CVkPipelineCache> CreatePipelineCache() const;

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

	TDataCache<GraphicsPipelineDesc, wil::com_ptr<CVkPipeline>, MaxResources> m_graphicsPipelineCache;
	std::mutex m_shaderModuleMutex;
	std::map<size_t, wil::com_ptr<CVkShaderModule>> m_shaderModuleHashMap;
	wil::com_ptr<CVkPipelineCache> m_pipelineCache;

	TDataCache<RootSignatureDesc, RootSignatureData, MaxResources> m_rootSignatureCache;
	std::mutex m_pipelineLayoutMutex;
	std::map<size_t, wil::com_ptr<CVkPipelineLayout>> m_pipelineLayoutHashMap;
};


ResourceManager* const GetVulkanResourceManager();

} // namespace Luna::VK