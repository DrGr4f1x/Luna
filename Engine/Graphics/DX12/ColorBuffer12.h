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
#include "Graphics\DX12\DirectXCommon.h"

using namespace Microsoft::WRL;


namespace Luna::DX12
{

class __declspec(uuid("9F27F827-B4AA-44CA-84AE-CBE99F8F2EF4")) IColorBuffer12 : public IColorBuffer
{
public:
	virtual ~IColorBuffer12() = default;

	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const noexcept = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const noexcept = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUAV(uint32_t uavIndex) const noexcept = 0;
};


class __declspec(uuid("FA13EEEA-C815-4D64-B0B6-7173D5C6D1E0")) ColorBuffer
	: RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IColorBuffer12, IColorBuffer, IPixelBuffer, IGpuImage>>
	, NonCopyable
{
	friend class GraphicsDevice;

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

	// IColorBuffer implementation
	Color GetClearColor() const noexcept { return m_clearColor; }

	// IColorBuffer12 implementation
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const noexcept final { return m_srvHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const noexcept final { return m_rtvHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetUAV(uint32_t uavIndex) const noexcept final { return m_uavHandles[uavIndex]; }

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

	// ColorBuffer data
	Color m_clearColor;

	// Pre-constructed descriptors
	D3D12_CPU_DESCRIPTOR_HANDLE m_srvHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHandle;
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 12> m_uavHandles;
};

} // namespace Luna::DX12