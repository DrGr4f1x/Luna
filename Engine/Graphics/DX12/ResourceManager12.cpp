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

#include "ResourceManager12.h"

using namespace std;


namespace Luna::DX12
{

ResourceManager* g_resourceManager{ nullptr };


ResourceManager::ResourceManager(ID3D12Device* device, D3D12MA::Allocator* allocator)
	: TResourceManager{ device, allocator }
{
	assert(g_resourceManager == nullptr);

	g_resourceManager = this;
}


ResourceManager::~ResourceManager()
{
	g_resourceManager = nullptr;
}


ResourceHandle ResourceManager::CreateDescriptorSet(const DescriptorSetDesc& descriptorSetDesc)
{
	return m_descriptorSetFactory.CreateDescriptorSet(descriptorSetDesc);
}


ResourceHandle ResourceManager::CreateDescriptorSet(const ResourceHandleType* handle, uint32_t rootParamIndex)
{
	const auto& rootSignatureDesc = GetRootSignatureDesc(handle);

	assert(rootParamIndex < rootSignatureDesc.rootParameters.size());

	const auto& rootParam = rootSignatureDesc.rootParameters[rootParamIndex];

	const bool isRootBuffer = rootParam.parameterType == RootParameterType::RootCBV ||
		rootParam.parameterType == RootParameterType::RootSRV ||
		rootParam.parameterType == RootParameterType::RootUAV;

	const bool isSamplerTable = rootParam.IsSamplerTable();

	const D3D12_DESCRIPTOR_HEAP_TYPE heapType = isSamplerTable
		? D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
		: D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	DescriptorSetDesc descriptorSetDesc{
		.descriptorHandle	= isRootBuffer ? DescriptorHandle{} : AllocateUserDescriptor(heapType),
		.numDescriptors		= rootParam.GetNumDescriptors(),
		.isSamplerTable		= isSamplerTable,
		.isRootBuffer		= isRootBuffer
	};

	return CreateDescriptorSet(descriptorSetDesc);
}


ResourceHandle ResourceManager::CreateColorBufferFromSwapChain(IDXGISwapChain* swapChain, uint32_t imageIndex)
{
	return m_colorBufferFactory.CreateColorBufferFromSwapChain(swapChain, imageIndex);
}


ID3D12Resource* ResourceManager::GetResource(const ResourceHandleType* handle) const
{
	const auto [index, type] = UnpackHandle(handle);

	switch (type)
	{
	case IResourceManager::ManagedColorBuffer:
		return m_colorBufferFactory.GetResource(index);

	case IResourceManager::ManagedDepthBuffer:
		return m_depthBufferFactory.GetResource(index);

	case IResourceManager::ManagedGpuBuffer:
		return m_gpuBufferFactory.GetResource(index);

	default:
		assert(false);
		return nullptr;
	}
}


D3D12_CPU_DESCRIPTOR_HANDLE ResourceManager::GetSRV(const ResourceHandleType* handle, bool depthSrv) const
{
	const auto [index, type] = UnpackHandle(handle);

	switch (type)
	{
	case IResourceManager::ManagedColorBuffer: 
		return m_colorBufferFactory.GetSRV(index);

	case IResourceManager::ManagedDepthBuffer: 
		return m_depthBufferFactory.GetSRV(index, depthSrv);

	case IResourceManager::ManagedGpuBuffer: 
		return m_gpuBufferFactory.GetSRV(index);

	default: 
		assert(false);
		return D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = 0 };
	}
}


D3D12_CPU_DESCRIPTOR_HANDLE ResourceManager::GetRTV(const ResourceHandleType* handle) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == IResourceManager::ManagedColorBuffer);

	return m_colorBufferFactory.GetRTV(index);
}


D3D12_CPU_DESCRIPTOR_HANDLE ResourceManager::GetDSV(const ResourceHandleType* handle, DepthStencilAspect depthStencilAspect) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == IResourceManager::ManagedDepthBuffer);

	return m_depthBufferFactory.GetDSV(index, depthStencilAspect);
}


D3D12_CPU_DESCRIPTOR_HANDLE ResourceManager::GetUAV(const ResourceHandleType* handle, uint32_t uavIndex) const
{
	const auto [index, type] = UnpackHandle(handle);

	switch (type)
	{
	case IResourceManager::ManagedColorBuffer: 
		return m_colorBufferFactory.GetUAV(index, uavIndex);

	case IResourceManager::ManagedGpuBuffer: 
		return m_gpuBufferFactory.GetUAV(index);

	default:
		assert(false);
		return D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = 0 };
	}
}


D3D12_CPU_DESCRIPTOR_HANDLE ResourceManager::GetCBV(const ResourceHandleType* handle) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == IResourceManager::ManagedGpuBuffer);

	return m_gpuBufferFactory.GetCBV(index);
}


uint64_t ResourceManager::GetGpuAddress(const ResourceHandleType* handle) const
{
	auto resource = GetResource(handle);

	if (resource)
	{
		return resource->GetGPUVirtualAddress();
	}

	assert(false);
	return D3D12_GPU_VIRTUAL_ADDRESS_NULL;
}


ID3D12PipelineState* ResourceManager::GetGraphicsPipelineState(const ResourceHandleType* handle) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == IResourceManager::ManagedGraphicsPipeline);

	return m_pipelineStateFactory.GetGraphicsPipelineState(index);
}


ID3D12RootSignature* ResourceManager::GetRootSignature(const ResourceHandleType* handle) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == IResourceManager::ManagedRootSignature);

	return m_rootSignatureFactory.GetRootSignature(index);
}


uint32_t ResourceManager::GetDescriptorTableBitmap(const ResourceHandleType* handle) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == IResourceManager::ManagedRootSignature);

	return m_rootSignatureFactory.GetDescriptorTableBitmap(index);
}


uint32_t ResourceManager::GetSamplerTableBitmap(const ResourceHandleType* handle) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == IResourceManager::ManagedRootSignature);

	return m_rootSignatureFactory.GetSamplerTableBitmap(index);
}


const vector<uint32_t>& ResourceManager::GetDescriptorTableSize(const ResourceHandleType* handle) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == IResourceManager::ManagedRootSignature);

	return m_rootSignatureFactory.GetDescriptorTableSizes(index);
}


bool ResourceManager::HasBindableDescriptors(const ResourceHandleType* handle) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == IResourceManager::ManagedDescriptorSet);

	return m_descriptorSetFactory.HasBindableDescriptors(index);
}


D3D12_GPU_DESCRIPTOR_HANDLE ResourceManager::GetGpuDescriptorHandle(const ResourceHandleType* handle) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == IResourceManager::ManagedDescriptorSet);

	return m_descriptorSetFactory.GetGpuDescriptorHandle(index);
}


uint64_t ResourceManager::GetDescriptorSetGpuAddress(const ResourceHandleType* handle) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == IResourceManager::ManagedDescriptorSet);

	return m_descriptorSetFactory.GetDescriptorSetGpuAddress(index);
}


uint64_t ResourceManager::GetDynamicOffset(const ResourceHandleType* handle) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == IResourceManager::ManagedDescriptorSet);

	return m_descriptorSetFactory.GetDynamicOffset(index);
}


uint64_t ResourceManager::GetGpuAddressWithOffset(const ResourceHandleType* handle) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == IResourceManager::ManagedDescriptorSet);

	return m_descriptorSetFactory.GetGpuAddressWithOffset(index);
}


ResourceManager* const GetD3D12ResourceManager()
{
	return g_resourceManager;
}

} // namespace Luna::DX12