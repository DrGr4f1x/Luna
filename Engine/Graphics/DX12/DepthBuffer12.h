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


class __declspec(uuid("48CAA6ED-3C56-4C5D-B7E2-12E80421FA32")) IDepthBuffer12 : public IDepthBuffer
{
public:
	virtual ID3D12Resource* GetResource() const noexcept = 0;
	virtual uint64_t GetGpuAddress() const noexcept = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const noexcept = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDSV_DepthReadOnly() const noexcept = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDSV_StencilReadOnly() const noexcept = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDSV_ReadOnly() const noexcept = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetDepthSRV() const noexcept = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetStencilSRV() const noexcept = 0;
};


class __declspec(uuid("F1A14567-A2A4-4B61-BCB9-C6B436F456C8")) DepthBuffer12 final
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IDepthBuffer12, IDepthBuffer, IPixelBuffer, IGpuResource>>
	, NonCopyable
{
public:
	DepthBuffer12(const DepthBufferDesc& depthBufferDesc, const DepthBufferDescExt& depthBufferDescExt);

	// IGpuResource implementation
	const std::string& GetName() const override { return m_name; }
	ResourceType GetResourceType() const noexcept override { return m_resourceType; }

	ResourceState GetUsageState() const noexcept override { return m_usageState; }
	void SetUsageState(ResourceState usageState) noexcept override { m_usageState = usageState; }
	ResourceState GetTransitioningState() const noexcept override { return m_transitioningState; }
	void SetTransitioningState(ResourceState transitioningState) noexcept override { m_transitioningState = transitioningState; }

	NativeObjectPtr GetNativeObject(NativeObjectType type, uint32_t index = 0) const noexcept override;

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

	// IDepthBuffer12 implementation
	ID3D12Resource* GetResource() const noexcept override { return m_resource.get(); }
	uint64_t GetGpuAddress() const noexcept override { return m_resource->GetGPUVirtualAddress(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const noexcept override { return m_dsvHandles[0]; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSV_DepthReadOnly() const noexcept override { return m_dsvHandles[1]; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSV_StencilReadOnly() const noexcept override { return m_dsvHandles[2]; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetDSV_ReadOnly() const noexcept override { return m_dsvHandles[3]; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetDepthSRV() const noexcept override { return m_depthSrvHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetStencilSRV() const noexcept override { return m_stencilSrvHandle; }

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

	// DirectX 12 data
	wil::com_ptr<ID3D12Resource> m_resource;

	// Pre-constructed descriptors
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 4> m_dsvHandles{};
	D3D12_CPU_DESCRIPTOR_HANDLE m_depthSrvHandle{};
	D3D12_CPU_DESCRIPTOR_HANDLE m_stencilSrvHandle{};
};

} // namespace Luna::DX12