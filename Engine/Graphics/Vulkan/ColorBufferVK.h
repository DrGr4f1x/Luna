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
#include "Graphics\ColorBuffer.h"
#include "Graphics\Vulkan\VulkanCommon.h"

using namespace Microsoft::WRL;


namespace Luna::VK
{

struct ColorBufferDescExt
{
	CVkImage* image{ nullptr };
	CVkImageView* imageViewRtv{ nullptr };
	CVkImageView* imageViewSrv{ nullptr };
	VkDescriptorImageInfo imageInfoSrv{};
	VkDescriptorImageInfo imageInfoUav{};
	ResourceState usageState{ ResourceState::Undefined };

	ColorBufferDescExt& SetImage(CVkImage* value) noexcept { image = value; return *this; }
	ColorBufferDescExt& SetImageViewRtv(CVkImageView* value) noexcept { imageViewRtv = value; return *this; }
	ColorBufferDescExt& SetImageViewSrv(CVkImageView* value) noexcept { imageViewSrv = value; return *this; }
	ColorBufferDescExt& SetImageInfoSrv(const VkDescriptorImageInfo& value) noexcept { imageInfoSrv = value; return *this; }
	ColorBufferDescExt& SetImageInfoUav(const VkDescriptorImageInfo& value) noexcept { imageInfoUav = value; return *this; }
	ColorBufferDescExt& SetUsageState(const ResourceState value) noexcept { usageState = value; return *this; }
};


class __declspec(uuid("584039DC-50BF-46BC-91F0-3675DE05ECE5")) IColorBufferVK : public IColorBuffer
{
public:
	virtual VkImageView GetImageViewRTV() const noexcept = 0;
	virtual VkImageView GetImageViewSRV() const noexcept = 0;
	virtual VkDescriptorImageInfo GetSRVImageInfo() const noexcept = 0;
	virtual VkDescriptorImageInfo GetUAVImageInfo() const noexcept = 0;
};


class __declspec(uuid("FCB01FE8-94C7-494D-AAC7-0DFC3B9248E7")) ColorBuffer
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IColorBufferVK, IColorBuffer, IPixelBuffer, IGpuImage>>
	, public NonCopyable
{
public:
	ColorBuffer(const ColorBufferDesc& desc, const ColorBufferDescExt& descExt);
	~ColorBuffer() final = default;

	// IObject implementation
	ResourceState GetUsageState() const noexcept final { return m_usageState; }
	void SetUsageState(ResourceState usageState) noexcept final { m_usageState = usageState; }
	ResourceState GetTransitioningState() const noexcept final { return m_transitioningState; }
	void SetTransitioningState(ResourceState transitioningState) noexcept final { m_transitioningState = transitioningState; }
	ResourceType GetResourceType() const noexcept final { return m_resourceType; }
	NativeObjectPtr GetNativeObject(NativeObjectType nativeObjectType) const noexcept final;

	// IPixelBuffer implementation
	uint64_t GetWidth() const noexcept override { return m_width; }
	uint32_t GetHeight() const noexcept override { return m_height; }
	uint32_t GetDepth() const noexcept override { return m_resourceType == ResourceType::Texture3D ? m_arraySizeOrDepth : 1; }
	uint32_t GetArraySize() const noexcept override { return m_resourceType == ResourceType::Texture3D ? 1 : m_arraySizeOrDepth; }
	uint32_t GetNumMips() const noexcept override { return m_numMips; }
	uint32_t GetNumSamples() const noexcept override { return m_numSamples; }
	Format GetFormat() const noexcept override { return m_format; }
	uint32_t GetPlaneCount() const noexcept override { return m_planeCount; }
	TextureDimension GetDimension() const noexcept override { return ResourceTypeToTextureDimension(m_resourceType); }

	// IColorBuffer implementation
	Color GetClearColor() const noexcept final { return m_clearColor; }

	// IColorBufferVK implementation
	VkImageView GetImageViewRTV() const noexcept final { return m_imageViewRtv->Get(); }
	VkImageView GetImageViewSRV() const noexcept final { return m_imageViewSrv->Get(); }
	VkDescriptorImageInfo GetSRVImageInfo() const noexcept final { return m_imageInfoSrv; }
	VkDescriptorImageInfo GetUAVImageInfo() const noexcept final { return m_imageInfoUav; }

private:
	std::string m_name;

	// GpuImage data
	wil::com_ptr<IVkImage> m_image;
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

	// ColorBuffer data
	Color m_clearColor;

	// ColorBufferVK data
	wil::com_ptr<IVkImageView> m_imageViewRtv;
	wil::com_ptr<IVkImageView> m_imageViewSrv;
	VkDescriptorImageInfo m_imageInfoSrv{};
	VkDescriptorImageInfo m_imageInfoUav{};
};


} // namespace Luna::VK