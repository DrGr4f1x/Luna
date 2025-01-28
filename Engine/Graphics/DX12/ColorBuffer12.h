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
#include "Graphics\DX12\GpuResource12.h"

using namespace Microsoft::WRL;


namespace Luna::DX12
{

struct ColorBufferDescExt
{
	ColorBufferDescExt() noexcept
	{
		rtvHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		srvHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		for (auto& handle : uavHandles)
		{
			handle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		}
	}

	ID3D12Resource* resource{ nullptr };
	uint32_t numFragments{ 1 };
	ResourceState usageState{ ResourceState::Undefined };
	uint8_t planeCount{ 1 };

	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle{};
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle{};
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 12> uavHandles{};

	constexpr ColorBufferDescExt& SetResource(ID3D12Resource* value) noexcept { resource = value; return *this; }
	constexpr ColorBufferDescExt& SetNumFragments(uint32_t value) noexcept { numFragments = value; return *this; }
	constexpr ColorBufferDescExt& SetUsageState(ResourceState value) noexcept { usageState = value; return *this; }
	constexpr ColorBufferDescExt& SetPlaneCount(uint8_t value) noexcept { planeCount = value; return *this; }
	constexpr ColorBufferDescExt& SetSrvHandle(D3D12_CPU_DESCRIPTOR_HANDLE value) noexcept { srvHandle = value; return *this; }
	constexpr ColorBufferDescExt& SetRtvHandle(D3D12_CPU_DESCRIPTOR_HANDLE value) noexcept { rtvHandle = value; return *this; }
	ColorBufferDescExt& SetUavHandles(const std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 12>& value) noexcept { uavHandles = value; return *this; }
};


class __declspec(uuid("3618277B-EA60-4D37-9904-B4256F66A36A")) IColorBufferData : public IGpuResourceData
{
public:
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const noexcept = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const noexcept = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUAV(uint32_t uavIndex) const noexcept = 0;
};


class __declspec(uuid("BBBCFA80-B6CE-484F-B710-AF72B424B26E")) ColorBufferData
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IColorBufferData, IGpuResourceData, IPlatformData>>
	, NonCopyable
{
	friend class GraphicsDevice;

public:
	explicit ColorBufferData(const ColorBufferDescExt& descExt);
	
	ID3D12Resource* GetResource() const noexcept override { return m_resource.get(); }
	uint64_t GetGpuAddress() const noexcept override { return m_resource->GetGPUVirtualAddress(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const noexcept override { return m_srvHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const noexcept override { return m_rtvHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetUAV(uint32_t uavIndex) const noexcept override { return m_uavHandles[uavIndex]; }

private:
	wil::com_ptr<ID3D12Resource> m_resource;

	// Pre-constructed descriptors
	D3D12_CPU_DESCRIPTOR_HANDLE m_srvHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHandle;
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 12> m_uavHandles;
};


class __declspec(uuid("4FD3B3F6-AEFA-482A-9D6D-1E41A7F8F52B")) IColorBuffer12 : public IColorBuffer
{
public:
	virtual ID3D12Resource* GetResource() const noexcept = 0;
	virtual uint64_t GetGpuAddress() const noexcept = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const noexcept = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const noexcept = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUAV(uint32_t uavIndex) const noexcept = 0;
};


class __declspec(uuid("01573791-0480-44D2-8F5F-4726DBC4232C")) ColorBuffer12 final
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IColorBuffer12, IColorBuffer, IPixelBuffer, IGpuResource>>
	, NonCopyable
{
	friend class DeviceManager;
	friend class GraphicsDevice;

public:
	ColorBuffer12(const ColorBufferDesc& desc, const ColorBufferDescExt& descExt);

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

	// IColorBuffer implementation
	Color GetClearColor() const noexcept override { return m_clearColor; }
	void SetClearColor(Color clearColor) noexcept override { m_clearColor = clearColor; }

	// IColorBuffer12 implementation
	ID3D12Resource* GetResource() const noexcept override { return m_resource.get(); }
	uint64_t GetGpuAddress() const noexcept override { return m_resource->GetGPUVirtualAddress(); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const noexcept override { return m_srvHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const noexcept override { return m_rtvHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetUAV(uint32_t uavIndex) const noexcept override { return m_uavHandles[uavIndex]; }

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

	// DirectX 12 data
	wil::com_ptr<ID3D12Resource> m_resource;

	// Pre-constructed descriptors
	D3D12_CPU_DESCRIPTOR_HANDLE m_srvHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHandle;
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 12> m_uavHandles;
};

} // namespace Luna::DX12