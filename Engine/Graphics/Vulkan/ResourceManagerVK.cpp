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

#include "ResourceManagerVK.h"

#include "FileSystem.h"

#include "DescriptorPoolVK.h"
#include "VulkanUtil.h"


using namespace std;


namespace Luna::VK
{

ResourceManager* g_resourceManager{ nullptr };


ResourceManager::ResourceManager(CVkDevice* device, CVmaAllocator* allocator)
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
	const auto [index, type] = UnpackHandle(handle);

	assert(type == ManagedRootSignature);

	const auto& rootSignatureDesc = GetRootSignatureDesc(handle);

	const auto& rootParam = rootSignatureDesc.rootParameters[rootParamIndex];

	const bool isDynamicBuffer = rootParam.parameterType == RootParameterType::RootCBV ||
		rootParam.parameterType == RootParameterType::RootSRV ||
		rootParam.parameterType == RootParameterType::RootUAV;

	DescriptorSetDesc descriptorSetDesc{
		.descriptorSetLayout	= m_rootSignatureFactory.GetDescriptorSetLayout(index, rootParamIndex),
		.rootParameter			= rootParam,
		.bindingOffsets			= rootSignatureDesc.bindingOffsets,
		.numDescriptors			= rootParam.GetNumDescriptors(),
		.isDynamicBuffer		= isDynamicBuffer
	};

	return CreateDescriptorSet(descriptorSetDesc);
}


ResourceHandle ResourceManager::CreateColorBufferFromSwapChainImage(CVkImage* swapChainImage, uint32_t width, uint32_t height, Format format, uint32_t imageIndex)
{
	return m_colorBufferFactory.CreateColorBufferFromSwapChainImage(swapChainImage, width, height, format, imageIndex);
}


VkImage ResourceManager::GetImage(const ResourceHandleType* handle) const
{
	const auto [index, type] = UnpackHandle(handle);

	switch (type)
	{
	case ManagedColorBuffer:
		return m_colorBufferFactory.GetImage(index);
		break;

	case ManagedDepthBuffer:
		return m_depthBufferFactory.GetImage(index);
		break;

	default:
		assert(false);
		return VK_NULL_HANDLE;
	}
}


VkBuffer ResourceManager::GetBuffer(const ResourceHandleType* handle) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == ManagedGpuBuffer);

	return m_gpuBufferFactory.GetBuffer(index);
}


VkImageView ResourceManager::GetImageViewSrv(const ResourceHandleType* handle) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == ManagedColorBuffer);

	return m_colorBufferFactory.GetImageViewSrv(index);
}


VkImageView ResourceManager::GetImageViewRtv(const ResourceHandleType* handle) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == ManagedColorBuffer);

	return m_colorBufferFactory.GetImageViewRtv(index);
}


VkImageView ResourceManager::GetImageViewDepth(const ResourceHandleType* handle, DepthStencilAspect depthStencilAspect) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == ManagedDepthBuffer);

	return m_depthBufferFactory.GetImageView(index, depthStencilAspect);
}


VkDescriptorImageInfo ResourceManager::GetImageInfoSrv(const ResourceHandleType* handle) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == ManagedColorBuffer);

	return m_colorBufferFactory.GetImageInfoSrv(index);
}


VkDescriptorImageInfo ResourceManager::GetImageInfoUav(const ResourceHandleType* handle) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == ManagedColorBuffer);

	return m_colorBufferFactory.GetImageInfoUav(index);
}


VkDescriptorImageInfo ResourceManager::GetImageInfoDepth(const ResourceHandleType* handle, bool depthSrv) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == ManagedDepthBuffer);

	return m_depthBufferFactory.GetImageInfo(index, depthSrv);
}


VkDescriptorBufferInfo ResourceManager::GetBufferInfo(const ResourceHandleType* handle) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == ManagedGpuBuffer);

	return m_gpuBufferFactory.GetBufferInfo(index);
}


VkBufferView ResourceManager::GetBufferView(const ResourceHandleType* handle) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == ManagedGpuBuffer);

	return m_gpuBufferFactory.GetBufferView(index);
}


VkPipeline ResourceManager::GetGraphicsPipeline(const ResourceHandleType* handle) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == ManagedGraphicsPipeline);

	return m_pipelineStateFactory.GetGraphicsPipeline(index);
}


VkPipelineLayout ResourceManager::GetPipelineLayout(const ResourceHandleType* handle) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == ManagedRootSignature);

	return m_rootSignatureFactory.GetPipelineLayout(index);
}


CVkDescriptorSetLayout* ResourceManager::GetDescriptorSetLayout(const ResourceHandleType* handle, uint32_t paramIndex) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == ManagedRootSignature);

	return m_rootSignatureFactory.GetDescriptorSetLayout(index, paramIndex);
}


const std::vector<DescriptorBindingDesc>& ResourceManager::GetLayoutBindings(const ResourceHandleType* handle, uint32_t paramIndex) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == ManagedRootSignature);

	return m_rootSignatureFactory.GetLayoutBindings(index, paramIndex);
}


bool ResourceManager::HasDescriptors(const ResourceHandleType* handle) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == ManagedDescriptorSet);

	return m_descriptorSetFactory.HasDescriptors(index);
}


VkDescriptorSet ResourceManager::GetDescriptorSet(const ResourceHandleType* handle) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == ManagedDescriptorSet);
	
	return m_descriptorSetFactory.GetDescriptorSet(index);
}


uint32_t ResourceManager::GetDynamicOffset(const ResourceHandleType* handle) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == ManagedDescriptorSet);
	
	return m_descriptorSetFactory.GetDynamicOffset(index);
}


bool ResourceManager::IsDynamicBuffer(const ResourceHandleType* handle) const
{
	const auto [index, type] = UnpackHandle(handle);

	assert(type == ManagedDescriptorSet);
	
	return m_descriptorSetFactory.IsDynamicBuffer(index);
}


std::tuple<uint32_t, uint32_t> ResourceManager::UnpackHandle(const ResourceHandleType* handle) const
{
	assert(handle != nullptr);

	const auto index = handle->GetIndex();
	const auto type = handle->GetType();

	return make_tuple(index, type);
}


ResourceManager* const GetVulkanResourceManager()
{
	return g_resourceManager;
}

} // namespace Luna::VK