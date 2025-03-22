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
#include "Graphics\RootSignature.h"
#include "Graphics\DX12\DescriptorAllocator12.h"
#include "Graphics\DX12\DirectXCommon.h"


namespace Luna::DX12
{

struct ResourceData
{
	wil::com_ptr<ID3D12Resource> resource;
	ResourceState usageState{ ResourceState::Undefined };
};


struct ColorBufferData
{
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 12> uavHandles;
	uint32_t planeCount{ 1 };
};


struct DepthBufferData
{
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 4> dsvHandles{};
	D3D12_CPU_DESCRIPTOR_HANDLE depthSrvHandle{};
	D3D12_CPU_DESCRIPTOR_HANDLE stencilSrvHandle{};
	uint32_t planeCount{ 1 };
};


struct GpuBufferData
{
	wil::com_ptr<D3D12MA::Allocation> allocation; // TODO: move this elsewhere, it's not the hot data
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle{};
	D3D12_CPU_DESCRIPTOR_HANDLE uavHandle{};
	D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle{};
};


struct RootSignatureData
{
	wil::com_ptr<ID3D12RootSignature> rootSignature;
	uint32_t descriptorTableBitmap{ 0 };
	uint32_t samplerTableBitmap{ 0 };
	std::vector<uint32_t> descriptorTableSizes;
};


struct DescriptorSetDesc
{
	DescriptorHandle descriptorHandle;
	uint32_t numDescriptors{ 0 };
	bool isSamplerTable{ false };
	bool isRootBuffer{ false };
};


// TODO: Put descriptorHandle, gpuAddress, and dynamicOffset into a separate struct - they're the hot data for the GPU.
struct DescriptorSetData
{
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, MaxDescriptorsPerTable> descriptors;
	DescriptorHandle descriptorHandle;
	uint64_t gpuAddress{ D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN };
	uint32_t numDescriptors{ 0 };
	uint32_t dirtyBits{ 0 };
	uint32_t dynamicOffset{ 0 };

	bool isSamplerTable{ false };
	bool isRootBuffer{ false };
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
		ManagedRootSignature		= 0x0010,
		ManagedDescriptorSet		= 0x0020,
	};

public:
	ResourceManager(ID3D12Device* device, D3D12MA::Allocator* allocator);
	~ResourceManager();

	// Creation/destruction methods
	ResourceHandle CreateColorBuffer(const ColorBufferDesc& colorBufferDesc) override;
	ResourceHandle CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc) override;
	ResourceHandle CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc) override;
	ResourceHandle CreateGraphicsPipeline(const GraphicsPipelineDesc& pipelineDesc) override;
	ResourceHandle CreateRootSignature(const RootSignatureDesc& rootSignatureDesc) override;
	ResourceHandle CreateDescriptorSet(const DescriptorSetDesc& descriptorSetDesc);
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
	ResourceHandle CreateDescriptorSet(const ResourceHandleType* handle, uint32_t rootParamIndex) override;

	// Descriptor set methods
	void SetSRV(const ResourceHandleType* handle, int slot, const ColorBuffer& colorBuffer) override;
	void SetSRV(const ResourceHandleType* handle, int slot, const DepthBuffer& depthBuffer, bool depthSrv = true) override;
	void SetSRV(const ResourceHandleType* handle, int slot, const GpuBuffer& gpuBuffer) override;
	void SetUAV(const ResourceHandleType* handle, int slot, const ColorBuffer& colorBuffer, uint32_t uavIndex = 0) override;
	void SetUAV(const ResourceHandleType* handle, int slot, const DepthBuffer& depthBuffer) override;
	void SetUAV(const ResourceHandleType* handle, int slot, const GpuBuffer& gpuBuffer) override;
	void SetCBV(const ResourceHandleType* handle, int slot, const GpuBuffer& gpuBuffer) override;
	void SetDynamicOffset(const ResourceHandleType* handle, uint32_t offset) override;
	void UpdateGpuDescriptors(const ResourceHandleType* handle) override;

	// Platform-specific methods
	ResourceHandle CreateColorBufferFromSwapChain(IDXGISwapChain* swapChain, uint32_t imageIndex);
	ID3D12Resource* GetResource(const ResourceHandleType* handle) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRV(const ResourceHandleType* handle, bool depthSrv) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTV(const ResourceHandleType* handle) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSV(const ResourceHandleType* handle, DepthStencilAspect depthStencilAspect) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetUAV(const ResourceHandleType* handle, uint32_t uavIndex = 0) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCBV(const ResourceHandleType* handle) const;
	uint64_t GetGpuAddress(const ResourceHandleType* handle) const;
	ID3D12PipelineState* GetGraphicsPipelineState(const ResourceHandleType* handle) const;
	ID3D12RootSignature* GetRootSignature(const ResourceHandleType* handle) const;
	uint32_t GetDescriptorTableBitmap(const ResourceHandleType* handle) const;
	uint32_t GetSamplerTableBitmap(const ResourceHandleType* handle) const;
	const std::vector<uint32_t>& GetDescriptorTableSize(const ResourceHandleType* handle) const;
	bool HasBindableDescriptors(const ResourceHandleType* handle) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(const ResourceHandleType* handle) const;
	uint64_t GetDescriptorSetGpuAddress(const ResourceHandleType* handle) const;
	uint64_t GetDynamicOffset(const ResourceHandleType* handle) const;
	uint64_t GetGpuAddressWithOffset(const ResourceHandleType* handle) const;

private:
	std::tuple<uint32_t, uint32_t, uint32_t> UnpackHandle(const ResourceHandleType* handle) const;
	std::pair<ResourceData, ColorBufferData> CreateColorBuffer_Internal(const ColorBufferDesc& colorBufferDesc);
	std::pair<ResourceData, DepthBufferData> CreateDepthBuffer_Internal(const DepthBufferDesc& depthBufferDesc);
	std::pair<ResourceData, GpuBufferData> CreateGpuBuffer_Internal(const GpuBufferDesc& gpuBufferDesc);
	wil::com_ptr<ID3D12PipelineState> CreateGraphicsPipeline_Internal(const GraphicsPipelineDesc& pipelineDesc);
	RootSignatureData CreateRootSignature_Internal(const RootSignatureDesc& rootSignatureDesc);
	wil::com_ptr<D3D12MA::Allocation> AllocateBuffer(const GpuBufferDesc& gpuBufferDesc) const;
	void SetDescriptor(DescriptorSetData& data, int slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);

private:
	wil::com_ptr<ID3D12Device> m_device;
	wil::com_ptr<D3D12MA::Allocator> m_allocator;

	// Allocation mutex
	std::mutex m_allocationMutex;

	// Resource freelist
	std::queue<uint32_t> m_resourceFreeList;

	// Resource indices
	std::array<uint32_t, MaxResources> m_resourceIndices;

	// Resource data
	std::array<ResourceData, MaxResources> m_resourceData;

	// Resource caches
	template<typename DescType, typename DataType, uint32_t MAX_ITEMS>
	struct TResourceCache
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

	TResourceCache<ColorBufferDesc, ColorBufferData, MaxResources> m_colorBufferCache;
	TResourceCache<DepthBufferDesc, DepthBufferData, MaxResources> m_depthBufferCache;
	TResourceCache<GpuBufferDesc, GpuBufferData, MaxResources> m_gpuBufferCache;
	
	TResourceCache<GraphicsPipelineDesc, wil::com_ptr<ID3D12PipelineState>, MaxResources> m_graphicsPipelineCache;
	std::mutex m_pipelineStateMutex;
	std::map<size_t, wil::com_ptr<ID3D12PipelineState>> m_pipelineStateMap;

	TResourceCache<RootSignatureDesc, RootSignatureData, MaxResources> m_rootSignatureCache;
	std::mutex m_rootSignatureMutex;
	std::map<size_t, wil::com_ptr<ID3D12RootSignature>> m_rootSignatureHashMap;

	TResourceCache<DescriptorSetDesc, DescriptorSetData, MaxResources> m_descriptorSetCache;
};


ResourceManager* const GetD3D12ResourceManager();

} // namespace Luna::DX12