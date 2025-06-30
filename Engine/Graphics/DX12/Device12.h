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
#include "Graphics\DX12\DirectXCommon.h"
#include "Graphics\DX12\DescriptorAllocator12.h"


namespace Luna::DX12
{

struct DescriptorSetDesc
{
	DescriptorHandle descriptorHandle;
	uint32_t numDescriptors{ 0 };
	bool isSamplerTable{ false };
	bool isRootBuffer{ false };
};


class Device : public IDevice
{
public:
	Device(ID3D12Device* device, D3D12MA::Allocator* allocator);

	ColorBufferPtr CreateColorBuffer(const ColorBufferDesc& colorBufferDesc) override;
	DepthBufferPtr CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc) override;
	GpuBufferPtr CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc) override;

	RootSignaturePtr CreateRootSignature(const RootSignatureDesc& rootSignatureDesc) override;

	GraphicsPipelineStatePtr CreateGraphicsPipelineState(const GraphicsPipelineDesc& pipelineDesc) override;

	DescriptorSetPtr CreateDescriptorSet(const DescriptorSetDesc& descriptorSetDesc);

	SamplerPtr CreateSampler(const SamplerDesc& samplerDesc) override;

	ITexture* CreateUninitializedTexture(const std::string& name, const std::string& mapKey) override;
	bool InitializeTexture(ITexture* texture, const TextureInitializer& texInit) override;

	ColorBufferPtr CreateColorBufferFromSwapChain(IDXGISwapChain* swapChain, uint32_t imageIndex);

	ID3D12Device* GetD3D12Device() { return m_device.get(); }

protected:
	wil::com_ptr<D3D12MA::Allocation> AllocateBuffer(const GpuBufferDesc& gpuBufferDesc) const;

protected:
	wil::com_ptr<ID3D12Device> m_device;
	wil::com_ptr<D3D12MA::Allocator> m_allocator;

	// Root signature cache
	std::mutex m_rootSignatureMutex;
	std::map<size_t, wil::com_ptr<ID3D12RootSignature>> m_rootSignatureHashMap;

	// Graphics pipeline state cache
	std::mutex m_graphicsPipelineStateMutex;
	std::map<size_t, wil::com_ptr<ID3D12PipelineState>> m_graphicsPipelineStateHashMap;

	// Sampler state cache
	std::mutex m_samplerMutex;
	std::map<size_t, D3D12_CPU_DESCRIPTOR_HANDLE> m_samplerMap;
};


Device* GetD3D12Device();

} // namespace Luna::DX12