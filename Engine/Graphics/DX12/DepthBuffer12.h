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

#include "Graphics\DepthBuffer.h"
#include "Graphics\DX12\DirectXCommon.h"

using namespace Microsoft::WRL;


namespace Luna::DX12
{

class __declspec(uuid("36E0E19C-7D07-46EA-A6FE-E222A083957D")) DepthBuffer
	: RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IDepthBuffer, IPixelBuffer, IGpuImage>>
	, NonCopyable
{
public:
	// IGpuResource implementation
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

private:
	ComPtr<ID3D12Resource> m_resource;
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
};

} // namespace Luna::DX12