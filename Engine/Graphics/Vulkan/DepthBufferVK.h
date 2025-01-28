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


class __declspec(uuid("1786B0B9-91B7-4580-81DE-163AB3248DD2")) IDepthBufferVK : public IDepthBuffer
{
public:
	virtual VkImage GetImage() const noexcept = 0;
	virtual VkImageView GetDepthStencilImageView() const noexcept = 0;
	virtual VkImageView GetDepthOnlyImageView() const noexcept = 0;
	virtual VkImageView GetStencilOnlyImageView() const noexcept = 0;
	virtual VkDescriptorImageInfo GetDepthImageInfo() const noexcept = 0;
	virtual VkDescriptorImageInfo GetStencilImageInfo() const noexcept = 0;
};


class __declspec(uuid("4DDCE68D-9D21-41DA-80D3-705DB153EBC2")) DepthBufferVK final
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IDepthBufferVK, IDepthBuffer, IPixelBuffer, IGpuResource>>
	, NonCopyable
{
public:
	DepthBufferVK(const DepthBufferDesc& depthBufferDesc, const DepthBufferDescExt& depthBufferDescExt);

	// IGpuResource implementation
	const std::string& GetName() const override { return m_name; }
	ResourceType GetResourceType() const noexcept override { return m_resourceType; }

	ResourceState GetUsageState() const noexcept override { return m_usageState; }
	void SetUsageState(ResourceState usageState) noexcept override { m_usageState = usageState; }
	ResourceState GetTransitioningState() const noexcept override { return m_transitioningState; }
	void SetTransitioningState(ResourceState transitioningState) noexcept override { m_transitioningState = transitioningState; }

	NativeObjectPtr GetNativeObject(NativeObjectType type, uint32_t index) const noexcept override;

	// IPixelBuffer implementation
	uint64_t GetWidth() const noexcept override { return m_width; }
	uint32_t GetHeight() const noexcept override { return m_height; }
	uint32_t GetDepth() const noexcept override { return m_arraySizeOrDepth; }
	uint32_t GetArraySize() const noexcept override { return m_arraySizeOrDepth; }
	uint32_t GetNumMips() const noexcept override { return m_numMips; }
	uint32_t GetNumSamples() const noexcept override { return m_numSamples; }
	uint32_t GetPlaneCount() const noexcept override { return m_planeCount; }
	Format GetFormat() const noexcept override { return m_format; }
	TextureDimension GetDimension() const noexcept override;

	// IDepthBuffer implementation
	float GetClearDepth() const noexcept override { return m_clearDepth; }
	uint8_t GetClearStencil() const noexcept override { return m_clearStencil; }

	// IDepthBufferVK
	VkImage GetImage() const noexcept override { return *m_image; }
	VkImageView GetDepthStencilImageView() const noexcept override { return *m_imageViewDepthStencil; }
	VkImageView GetDepthOnlyImageView() const noexcept override { return *m_imageViewDepthOnly; }
	VkImageView GetStencilOnlyImageView() const noexcept override { return *m_imageViewStencilOnly; }
	VkDescriptorImageInfo GetDepthImageInfo() const noexcept override { return m_imageInfoDepth; }
	VkDescriptorImageInfo GetStencilImageInfo() const noexcept override { return m_imageInfoStencil; }

private:
	std::string m_name;
	ResourceType m_resourceType{ ResourceType::Texture2D };

	ResourceState m_usageState{ ResourceState::Undefined };
	ResourceState m_transitioningState{ ResourceState::Undefined };

	uint64_t m_width{ 0 };
	uint32_t m_height{ 0 };
	uint32_t m_arraySizeOrDepth{ 0 };
	uint32_t m_numMips{ 0 };
	uint32_t m_numSamples{ 0 };
	uint32_t m_planeCount{ 1 };
	Format m_format{ Format::Unknown };

	float m_clearDepth{ 1.0f };
	uint8_t m_clearStencil{ 0 };

	wil::com_ptr<CVkImage> m_image;
	wil::com_ptr<CVkImageView> m_imageViewDepthStencil;
	wil::com_ptr<CVkImageView> m_imageViewDepthOnly;
	wil::com_ptr<CVkImageView> m_imageViewStencilOnly;
	VkDescriptorImageInfo m_imageInfoDepth{};
	VkDescriptorImageInfo m_imageInfoStencil{};
};

} // namespace Luna::VK