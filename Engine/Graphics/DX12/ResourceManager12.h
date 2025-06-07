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

#include "Graphics\ResourceManager.h"

#include "Graphics\DX12\ColorBufferFactory12.h"
#include "Graphics\DX12\DepthBufferFactory12.h"
#include "Graphics\DX12\DescriptorAllocator12.h"
#include "Graphics\DX12\DescriptorSetFactory12.h"
#include "Graphics\DX12\DirectXCommon.h"
#include "Graphics\DX12\GpuBufferFactory12.h"
#include "Graphics\DX12\PipelineStateFactory12.h"
#include "Graphics\DX12\RootSignatureFactory12.h"


namespace Luna::DX12
{

class ResourceManager 
	: public TResourceManager<
		ID3D12Device,
		D3D12MA::Allocator,
		ColorBufferFactory,
		DepthBufferFactory,
		GpuBufferFactory,
		PipelineStateFactory,
		RootSignatureFactory,
		DescriptorSetFactory>
{
public:
	ResourceManager(ID3D12Device* device, D3D12MA::Allocator* allocator);
	~ResourceManager();

	// Creation/destruction methods
	ResourceHandle CreateDescriptorSet(const DescriptorSetDesc& descriptorSetDesc);

	// Root signature methods
	ResourceHandle CreateDescriptorSet(const ResourceHandleType* handle, uint32_t rootParamIndex) override;

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
	std::pair<uint32_t, uint32_t> UnpackHandle(const ResourceHandleType* handle) const
	{
		return std::make_pair<uint32_t, uint32_t>(handle->GetIndex(), handle->GetType());
	}
};


ResourceManager* const GetD3D12ResourceManager();

} // namespace Luna::DX12