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


class __declspec(uuid("0F5638CA-C7FC-421C-AF8F-CE144FE21485")) IColorBufferVK : public IColorBuffer
{
public:
	virtual VkImage GetImage() const noexcept = 0;
	virtual VkImageView GetImageViewRtv() const noexcept = 0;
	virtual VkImageView GetImageViewSrv() const noexcept = 0;
	virtual VkDescriptorImageInfo GetImageInfoSrv() const noexcept = 0;
	virtual VkDescriptorImageInfo GetImageInfoUav() const noexcept = 0;
};


class __declspec(uuid("E8EB9983-8DFF-417A-B062-56F578501BCA")) ColorBufferVK final
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IColorBufferVK, IColorBuffer, IPixelBuffer, IGpuResource>>
	, NonCopyable
{
public:
	ColorBufferVK(const ColorBufferDesc& desc, const ColorBufferDescExt& descExt);

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

	// IColorBuffer implementation
	Color GetClearColor() const noexcept override { return m_clearColor; }
	void SetClearColor(Color clearColor) noexcept override { m_clearColor = clearColor; }

	// IColorBufferVK implementation
	VkImage GetImage() const noexcept override { return *m_image; }
	VkImageView GetImageViewRtv() const noexcept override { return *m_imageViewRtv; }
	VkImageView GetImageViewSrv() const noexcept override { return *m_imageViewSrv; }
	VkDescriptorImageInfo GetImageInfoSrv() const noexcept override { return m_imageInfoSrv; }
	VkDescriptorImageInfo GetImageInfoUav() const noexcept override { return m_imageInfoUav; }

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

	Color m_clearColor{ DirectX::Colors::Black };

	// Vulkan data
	wil::com_ptr<CVkImage> m_image;
	wil::com_ptr<CVkImageView> m_imageViewRtv;
	wil::com_ptr<CVkImageView> m_imageViewSrv;
	VkDescriptorImageInfo m_imageInfoSrv{};
	VkDescriptorImageInfo m_imageInfoUav{};
};

} // namespace Luna::VK