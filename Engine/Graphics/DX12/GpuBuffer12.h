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

#include "Graphics\GpuBuffer.h"
#include "Graphics\DX12\DirectXCommon.h"
#include "Graphics\DX12\GpuResource12.h"

using namespace Microsoft::WRL;


namespace Luna::DX12
{

struct GpuBufferDescExt
{
	ID3D12Resource* resource{ nullptr };
	D3D12MA::Allocation* allocation{ nullptr };
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle{};
	D3D12_CPU_DESCRIPTOR_HANDLE uavHandle{};
	ResourceState usageState{ ResourceState::Undefined };

	constexpr GpuBufferDescExt& SetResource(ID3D12Resource* value) noexcept { resource = value; return *this; }
	constexpr GpuBufferDescExt& SetAllocation(D3D12MA::Allocation* value) noexcept { allocation = value; return *this; }
	constexpr GpuBufferDescExt& SetSrvHandle(D3D12_CPU_DESCRIPTOR_HANDLE value) noexcept { srvHandle = value; return *this; }
	constexpr GpuBufferDescExt& SetUavHandle(D3D12_CPU_DESCRIPTOR_HANDLE value) noexcept { uavHandle = value; return *this; }
	constexpr GpuBufferDescExt& SetUsageState(ResourceState value) noexcept { usageState = value; return *this; }
};


class __declspec(uuid("9138A2AC-37DF-4B04-91B7-293B8134CCCF")) IGpuBufferData : public IGpuResourceData
{
public:
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const noexcept = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUAV() const noexcept = 0;

	virtual D3D12_VERTEX_BUFFER_VIEW VertexBufferView(size_t offset, uint32_t size, uint32_t stride) const noexcept = 0;
	virtual D3D12_VERTEX_BUFFER_VIEW VertexBufferView(size_t baseVertexIndex) const noexcept = 0;

	virtual D3D12_INDEX_BUFFER_VIEW IndexBufferView(size_t offset, uint32_t size, bool b32Bit = false) const noexcept = 0;
	virtual D3D12_INDEX_BUFFER_VIEW IndexBufferView(size_t startIndex = 0) const noexcept = 0;
};


class __declspec(uuid("7BCB7DD3-EDEE-4CBD-9C4B-3D32159EE496")) GpuBufferData final
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IGpuBufferData, IGpuResourceData, IPlatformData>>
	, NonCopyable
{
public:
	explicit GpuBufferData(const GpuBufferDesc& desc, const GpuBufferDescExt& descExt);

	ID3D12Resource* GetResource() const noexcept override { return m_resource.get(); }
	uint64_t GetGpuAddress() const noexcept override { return m_gpuAddress; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const noexcept override { return m_srvHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetUAV() const noexcept override { return m_uavHandle; }
	
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView(size_t offset, uint32_t size, uint32_t stride) const noexcept override;
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView(size_t baseVertexIndex) const noexcept override
	{
		size_t offset = baseVertexIndex + m_elementSize;
		return VertexBufferView(offset, (uint32_t)(m_bufferSize - offset), (uint32_t)m_elementSize);

	}

	D3D12_INDEX_BUFFER_VIEW IndexBufferView(size_t offset, uint32_t size, bool b32Bit = false) const noexcept override;
	D3D12_INDEX_BUFFER_VIEW IndexBufferView(size_t startIndex = 0) const noexcept override
	{
		size_t offset = startIndex * m_elementSize;
		return IndexBufferView(offset, (uint32_t)(m_bufferSize - offset), m_elementSize == 4);
	}

protected:
	wil::com_ptr<ID3D12Resource> m_resource;
	wil::com_ptr<D3D12MA::Allocation> m_allocation;
	D3D12_CPU_DESCRIPTOR_HANDLE m_srvHandle{};
	D3D12_CPU_DESCRIPTOR_HANDLE m_uavHandle{};

	Format m_format{ Format::Unknown };

	size_t m_bufferSize{ 0 };
	size_t m_elementCount{ 0 };
	size_t m_elementSize{ 0 };
	uint64_t m_gpuAddress{ 0 };
};


inline D3D12_VERTEX_BUFFER_VIEW GpuBufferData::VertexBufferView(size_t offset, uint32_t size, uint32_t stride) const noexcept
{
	return D3D12_VERTEX_BUFFER_VIEW{
		.BufferLocation		= m_gpuAddress + offset,
		.SizeInBytes		= size,
		.StrideInBytes		= stride
	};
}


inline D3D12_INDEX_BUFFER_VIEW GpuBufferData::IndexBufferView(size_t offset, uint32_t size, bool b32Bit) const noexcept
{
	return D3D12_INDEX_BUFFER_VIEW{
		.BufferLocation		= m_gpuAddress + offset,
		.SizeInBytes		= size,
		.Format				= b32Bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT
	};
}


class __declspec(uuid("D6D83D05-B88B-4E1C-BFA0-C82B2C48AF87")) GpuBuffer12 final
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IGpuBuffer, IGpuResource>>
	, NonCopyable
{
public:
	GpuBuffer12(const GpuBufferDesc& gpuBufferDesc, const GpuBufferDescExt& gpuBufferDescExt);

	// IGpuResource implementation
	const std::string& GetName() const override { return m_name; }
	ResourceType GetResourceType() const noexcept override { return m_resourceType; }

	ResourceState GetUsageState() const noexcept override { return m_usageState; }
	void SetUsageState(ResourceState usageState) noexcept override { m_usageState = usageState; }
	ResourceState GetTransitioningState() const noexcept override { return m_transitioningState; }
	void SetTransitioningState(ResourceState transitioningState) noexcept override { m_transitioningState = transitioningState; }

	NativeObjectPtr GetNativeObject(NativeObjectType type, uint32_t index = 0) const noexcept override;

	// IGpuBuffer implementation
	size_t GetSize() const noexcept override { return m_elementCount * m_elementSize; }
	size_t GetElementCount() const noexcept override { return m_elementCount; }
	size_t GetElementSize() const noexcept override { return m_elementSize; }

private:
	std::string m_name;
	ResourceType m_resourceType{ ResourceType::Texture2D };

	ResourceState m_usageState{ ResourceState::Undefined };
	ResourceState m_transitioningState{ ResourceState::Undefined };

	size_t m_elementCount{ 0 };
	size_t m_elementSize{ 0 };

	wil::com_ptr<ID3D12Resource> m_resource;
	wil::com_ptr<D3D12MA::Allocation> m_allocation;
	D3D12_CPU_DESCRIPTOR_HANDLE m_srvHandle{};
	D3D12_CPU_DESCRIPTOR_HANDLE m_uavHandle{};
};

} // namespace Luna::DX12