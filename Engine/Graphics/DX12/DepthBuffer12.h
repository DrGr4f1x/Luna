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

struct DepthBufferDescExt
{
	DepthBufferDescExt()
	{
		for (auto& handle : dsvHandles)
		{
			handle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		}
		depthSrvHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		stencilSrvHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	ID3D12Resource* resource{ nullptr };
	ResourceState usageState{ ResourceState::Undefined };
	uint8_t planeCount{ 1 };

	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 4> dsvHandles{};
	D3D12_CPU_DESCRIPTOR_HANDLE depthSrvHandle{};
	D3D12_CPU_DESCRIPTOR_HANDLE stencilSrvHandle{};

	constexpr DepthBufferDescExt& SetResource(ID3D12Resource* value) noexcept { resource = value; return *this; }
	constexpr DepthBufferDescExt& SetUsageState(ResourceState value) noexcept { usageState = value; return *this; }
	constexpr DepthBufferDescExt& SetPlaneCount(uint8_t value) noexcept { planeCount = value; return *this; }
	DepthBufferDescExt& SetDsvHandles(const std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 4>& value) noexcept { dsvHandles = value; return *this; }
	constexpr DepthBufferDescExt& SetDepthSrvHandle(D3D12_CPU_DESCRIPTOR_HANDLE value) noexcept { depthSrvHandle = value; return *this; }
	constexpr DepthBufferDescExt& SetStencilSrvHandle(D3D12_CPU_DESCRIPTOR_HANDLE value) noexcept { stencilSrvHandle = value; return *this; }
};


class __declspec(uuid("232C36B6-60FC-4DE6-8AD5-84769D22CDFF")) IDepthBuffer12 : public IDepthBuffer
{
public:
	virtual ~IDepthBuffer12() = default;

	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const noexcept = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDSV_DepthReadOnly() const noexcept = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDSV_StencilReadOnly() const noexcept = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDSV_ReadOnly() const noexcept = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDepthSRV() const noexcept = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetStencilSRV() const noexcept = 0;
};


class __declspec(uuid("36E0E19C-7D07-46EA-A6FE-E222A083957D")) DepthBuffer
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IDepthBuffer12, IDepthBuffer, IPixelBuffer, IGpuImage>>
	, NonCopyable
{
public:
	DepthBuffer(const DepthBufferDesc& desc, const DepthBufferDescExt& descExt) noexcept;

	// IGpuResource implementation
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

	// IDEpthBuffer implementation
	float GetClearDepth() const noexcept final { return m_clearDepth; }
	uint8_t GetClearStencil() const noexcept final { return m_clearStencil; }

private:
	std::string m_name;

	wil::com_ptr<ID3D12Resource> m_resource;
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

	// DepthBuffer12 data
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 4> m_dsvHandles{};
	D3D12_CPU_DESCRIPTOR_HANDLE m_depthSrvHandle{};
	D3D12_CPU_DESCRIPTOR_HANDLE m_stencilSrvHandle{};
};

} // namespace Luna::DX12