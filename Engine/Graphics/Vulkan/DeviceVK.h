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
#include "Graphics\Vulkan\VulkanCommon.h"
#include "Graphics\Vulkan\DescriptorPoolVK.h"
#include "Graphics\Vulkan\RootSignatureVK.h"


namespace Luna
{

// Forward declarations
class Shader;

} // namespace Luna


namespace Luna::VK
{

struct DescriptorSetDesc
{
	CVkDescriptorSetLayout* descriptorSetLayout{ nullptr };
	RootParameter rootParameter{};
	uint32_t numDescriptors{ 0 };
	bool isDynamicBuffer{ false };
};


class Device : public IDevice
{
public:
	Device(CVkDevice* device, CVmaAllocator* allocator, const Luna::DeviceCaps& caps);

	const Luna::DeviceCaps& GetDeviceCaps() const override { return m_caps; }

	ColorBufferPtr CreateColorBuffer(const ColorBufferDesc& colorBufferDesc) override;
	DepthBufferPtr CreateDepthBuffer(const DepthBufferDesc& depthBufferDesc) override;
	GpuBufferPtr CreateGpuBuffer(const GpuBufferDesc& gpuBufferDesc) override;

	RootSignaturePtr CreateRootSignature(const RootSignatureDesc& rootSignatureDesc) override;

	GraphicsPipelinePtr CreateGraphicsPipeline(const GraphicsPipelineDesc& pipelineDesc) override;
	ComputePipelinePtr CreateComputePipeline(const ComputePipelineDesc& pipelineDesc) override;

	QueryHeapPtr CreateQueryHeap(const QueryHeapDesc& queryHeapDesc) override;

	DescriptorSetPtr CreateDescriptorSet(const DescriptorSetDesc& descriptorSetDesc);

	SamplerPtr CreateSampler(const SamplerDesc& samplerDesc) override;

	TexturePtr CreateTexture1D(const TextureDesc& textureDesc);
	TexturePtr CreateTexture2D(const TextureDesc& textureDesc);
	TexturePtr CreateTexture3D(const TextureDesc& textureDesc);

	ITexture* CreateUninitializedTexture(const std::string& name, const std::string& mapKey) override;
	bool InitializeTexture(ITexture* texture, const TextureInitializer& texInit) override;

	ColorBufferPtr CreateColorBufferFromSwapChainImage(CVkImage* swapChainImage, uint32_t width, uint32_t height, Format format, uint32_t imageIndex);

	VkDevice GetVulkanDevice() const { return m_device->Get(); }

protected:
	wil::com_ptr<CVkImage> CreateImage(const ImageDesc& imageDesc);
	wil::com_ptr<CVkImageView> CreateImageView(const ImageViewDesc& imageViewDesc);

	wil::com_ptr<CVkShaderModule> CreateShaderModule(Shader* shader);
	wil::com_ptr<CVkPipelineCache> CreatePipelineCache() const;

	void CreateDescriptorSetLayout(
		VkDescriptorSetLayout* setLayout,
		std::vector<DescriptorBindingDesc>& bindingDescs,
		const RootParameter& rootParameter,
		size_t& hashCode);

	TexturePtr CreateTextureSimple(TextureDimension dimension, const TextureDesc& textureDesc);

protected:
	wil::com_ptr<CVkDevice> m_device;
	wil::com_ptr<CVmaAllocator> m_allocator;

	// Device caps
	Luna::DeviceCaps m_caps{};

	// Pipeline layout cache (RootSignature)
	std::mutex m_pipelineLayoutMutex;
	std::map<size_t, wil::com_ptr<CVkPipelineLayout>> m_pipelineLayoutHashMap;

	// Shader modules
	std::mutex m_shaderModuleMutex;
	std::map<size_t, wil::com_ptr<CVkShaderModule>> m_shaderModuleHashMap;

	// Pipeline cache
	wil::com_ptr<CVkPipelineCache> m_pipelineCache;

	// Descriptor set cache
	std::mutex m_descriptorSetMutex;
	std::unordered_map<VkDescriptorSetLayout, std::unique_ptr<DescriptorPool>> m_setPoolMapping;

	// Sampler state cache
	std::mutex m_samplerMutex;
	std::unordered_map<size_t, wil::com_ptr<CVkSampler>> m_samplerMap;
};


Device* GetVulkanDevice();

} // namespace Luna::VK