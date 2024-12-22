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

#include "Core\Color.h"
#include "Graphics\DepthBuffer.h"
#include "Graphics\Vulkan\VulkanCommon.h"

using namespace Microsoft::WRL;


namespace Luna::VK
{

struct DepthBufferDescExt
{
	CVkImage* image{ nullptr };
	CVkImageView* imageViewDepthStencil{ nullptr };
	CVkImageView* imageViewDepthOnly{ nullptr };
	CVkImageView* imageViewStencilOnly{ nullptr };
	VkDescriptorImageInfo imageInfoDepth{};
	VkDescriptorImageInfo imageInfoStencil{};
	ResourceState usageState{ ResourceState::Undefined };

	DepthBufferDescExt& SetImage(CVkImage* value) noexcept { image = value; return *this; }
	DepthBufferDescExt& SetImageViewDepthStencil(CVkImageView* value) noexcept { imageViewDepthStencil = value; return *this; }
	DepthBufferDescExt& SetImageViewDepthOnly(CVkImageView* value) noexcept { imageViewDepthOnly = value; return *this; }
	DepthBufferDescExt& SetImageViewStencilOnly(CVkImageView* value) noexcept { imageViewStencilOnly = value; return *this; }
	DepthBufferDescExt& SetImageInfoDepth(const VkDescriptorImageInfo& value) noexcept { imageInfoDepth = value; return *this; }
	DepthBufferDescExt& SetImageInfoStencil(const VkDescriptorImageInfo& value) noexcept { imageInfoStencil = value; return *this; }
	DepthBufferDescExt& SetUsageState(const ResourceState value) noexcept { usageState = value; return *this; }
};


class __declspec(uuid("88609DFB-84C1-44D4-88DC-96B5CBB07556")) IDepthBufferVK : public IDepthBuffer
{
public:
	virtual ~IDepthBufferVK() = default;

	// Get pre-created CPU-visible descriptor handles
	virtual VkImageView GetDepthStencilImageView() const noexcept = 0;
	virtual VkImageView GetDepthOnlyImageView() const noexcept = 0;
	virtual VkImageView GetStencilOnlyImageView() const noexcept = 0;
	virtual VkDescriptorImageInfo GetDepthImageInfo() const noexcept = 0;
	virtual VkDescriptorImageInfo GetStencilImageInfo() const noexcept = 0;
};


class __declspec(uuid("7F056BBE-DB42-4BA1-BCBB-A4AADF7669CC")) DepthBuffer
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IDepthBufferVK, IDepthBuffer, IPixelBuffer, IGpuImage>>
	, public NonCopyable
{
public:
	DepthBuffer(const DepthBufferDesc& desc, const DepthBufferDescExt& descExt);
	virtual ~DepthBuffer() = default;

	// IObject implementation
	ResourceState GetUsageState() const noexcept final { return m_usageState; }
	void SetUsageState(ResourceState usageState) noexcept final { m_usageState = usageState; }
	ResourceState GetTransitioningState() const noexcept final { return m_transitioningState; }
	void SetTransitioningState(ResourceState transitioningState) noexcept final { m_transitioningState = transitioningState; }
	ResourceType GetResourceType() const noexcept final { return m_resourceType; }
	NativeObjectPtr GetNativeObject(NativeObjectType nativeObjectType) const noexcept final;

	// IPixelBuffer implementation
	uint64_t GetWidth() const noexcept final { return m_width; }
	uint32_t GetHeight() const noexcept final { return m_height; }
	uint32_t GetDepth() const noexcept final { return m_resourceType == ResourceType::Texture3D ? m_arraySizeOrDepth : 1; }
	uint32_t GetArraySize() const noexcept final { return m_resourceType == ResourceType::Texture3D ? 1 : m_arraySizeOrDepth; }
	uint32_t GetNumMips() const noexcept final { return m_numMips; }
	uint32_t GetNumSamples() const noexcept final { return m_numSamples; }
	Format GetFormat() const noexcept final { return m_format; }
	uint32_t GetPlaneCount() const noexcept final { return m_planeCount; }
	TextureDimension GetDimension() const noexcept final { return ResourceTypeToTextureDimension(m_resourceType); }

	// IDepthBuffer implementation
	float GetClearDepth() const noexcept final { return m_clearDepth; }
	uint8_t GetClearStencil() const noexcept final { return m_clearStencil; }

	// IDepthBufferVK implementation
	VkImageView GetDepthStencilImageView() const noexcept final { return *m_imageViewDepthStencil; }
	VkImageView GetDepthOnlyImageView() const noexcept final { return *m_imageViewDepthOnly; }
	VkImageView GetStencilOnlyImageView() const noexcept final { return *m_imageViewStencilOnly; }
	VkDescriptorImageInfo GetDepthImageInfo() const noexcept final { return m_imageInfoDepth; }
	VkDescriptorImageInfo GetStencilImageInfo() const noexcept final { return m_imageInfoStencil; }

private:
	std::string m_name;

	// GpuImage data
	wil::com_ptr<CVkImage> m_image;
	ResourceState m_usageState{ ResourceState::Undefined };
	ResourceState m_transitioningState{ ResourceState::Undefined };
	ResourceType m_resourceType{ ResourceType::Unknown };

	// PixelBuffer data
	uint64_t m_width{ 0 };
	uint32_t m_height{ 0 };
	uint32_t m_arraySizeOrDepth{ 0 };
	uint32_t m_numMips{ 1 };
	uint32_t m_numSamples{ 1 };
	uint32_t m_planeCount{ 1 };
	Format m_format{ Format::Unknown };

	// DepthBuffer data
	float m_clearDepth{ 1.0f };
	uint8_t m_clearStencil{ 0 };

	wil::com_ptr<CVkImageView> m_imageViewDepthStencil;
	wil::com_ptr<CVkImageView> m_imageViewDepthOnly;
	wil::com_ptr<CVkImageView> m_imageViewStencilOnly;
	VkDescriptorImageInfo m_imageInfoDepth{};
	VkDescriptorImageInfo m_imageInfoStencil{};
};

} // namespace Luna::VK