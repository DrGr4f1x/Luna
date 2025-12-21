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

#include "Graphics\Device.h"
#include "Graphics\DeviceCaps.h"
#include "Graphics\DX12\DirectXCommon.h"
#include "Graphics\DX12\Descriptor12.h"
#include "Graphics\DX12\DescriptorAllocator12.h"
#include "Graphics\DX12\Sampler12.h"


namespace Luna::DX12
{

// Forward declarations
struct GraphicsPipelineContext;


struct DescriptorSetDesc
{
	DescriptorHandle descriptorHandle;
	RootParameter rootParameter;
	uint32_t numDescriptors{ 0 };
	bool isSamplerTable{ false };
};


class Device : public IDevice
{
public:
	Device(ID3D12Device* device, D3D12MA::Allocator* allocator);

	const DeviceCaps& GetDeviceCaps() const override { return m_caps; }

	ColorBufferPtr CreateColorBuffer(const ColorBufferDesc& colorBufferDesc) override;
	DepthBufferPtr CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc) override;
	GpuBufferPtr CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc) override;

	RootSignaturePtr CreateRootSignature(const RootSignatureDesc& rootSignatureDesc) override;

	GraphicsPipelinePtr CreateGraphicsPipeline(const GraphicsPipelineDesc& pipelineDesc) override;
	ComputePipelinePtr CreateComputePipeline(const ComputePipelineDesc& pipelineDesc) override;
	MeshletPipelinePtr CreateMeshletPipeline(const MeshletPipelineDesc& pipelineDesc) override;

	QueryHeapPtr CreateQueryHeap(const QueryHeapDesc& queryHeapDesc) override;

	DescriptorSetPtr CreateDescriptorSet(const DescriptorSetDesc& descriptorSetDesc);

	SamplerPtr CreateSampler(const SamplerDesc& samplerDesc) override;

	TexturePtr CreateTexture1D(const TextureDesc& textureDesc);
	TexturePtr CreateTexture2D(const TextureDesc& textureDesc);
	TexturePtr CreateTexture3D(const TextureDesc& textureDesc);

	ITexture* CreateUninitializedTexture(const std::string& name, const std::string& mapKey) override;
	bool InitializeTexture(ITexture* texture, const TextureInitializer& texInit) override;

	ColorBufferPtr CreateColorBufferFromSwapChain(IDXGISwapChain* swapChain, uint32_t imageIndex);

	ID3D12Device* GetD3D12Device() { return m_device.get(); }

	// Caps
	void FillCaps(const AdapterInfo& adapterInfo);

	// CPU descriptors
	DescriptorHandle2 AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE heapType);
	void FreeDescriptorHandle(const DescriptorHandle2& handle);
	D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandleCPU(const DescriptorHandle2 handle);

protected:
	wil::com_ptr<D3D12MA::Allocation> AllocateBuffer(const GpuBufferDesc& gpuBufferDesc) const;
	TexturePtr CreateTextureSimple(TextureDimension dimension, const TextureDesc& textureDesc);

	template <class TPipelineStream>
	GraphicsPipelinePtr CreateGraphicsPipelineStream(TPipelineStream& pipelineStream, size_t hashCode, const GraphicsPipelineDesc& pipelineDesc);

	template <class TPipelineStream>
	MeshletPipelinePtr CreateMeshletPipelineStream(TPipelineStream& pipelineStream, size_t hashCode, const MeshletPipelineDesc& pipelineDesc);

protected:
	wil::com_ptr<ID3D12Device> m_device;
	wil::com_ptr<ID3D12Device2> m_device2;
	wil::com_ptr<D3D12MA::Allocator> m_allocator;

	// Device caps
	DeviceCaps m_caps{};

	// CPU descriptors
	std::array<std::mutex, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> m_freeDescriptorMutexes;
	std::mutex m_descriptorHeapLock;
	std::vector<std::vector<DescriptorHandle2>> m_freeDescriptors;
	std::vector<DescriptorHeapDesc> m_descriptorHeaps;

	// Root signature cache
	std::mutex m_rootSignatureMutex;
	std::map<size_t, wil::com_ptr<ID3D12RootSignature>> m_rootSignatureHashMap;

	// Graphics pipeline state cache
	std::mutex m_graphicsPipelineStateMutex;
	std::map<size_t, wil::com_ptr<ID3D12PipelineState>> m_graphicsPipelineStateHashMap;

	// Compute pipeline state cache
	std::mutex m_computePipelineStateMutex;
	std::map<size_t, wil::com_ptr<ID3D12PipelineState>> m_computePipelineStateHashMap;

	// Meshlet pipeline state cache
	std::mutex m_meshletPipelineStateMutex;
	std::map<size_t, wil::com_ptr<ID3D12PipelineState>> m_meshletPipelineStateHashMap;

	// Sampler state cache
	std::mutex m_samplerMutex;
	std::map<size_t, std::shared_ptr<Sampler>> m_samplerMap;
};


Device* GetD3D12Device();

} // namespace Luna::DX12