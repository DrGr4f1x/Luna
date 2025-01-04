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

struct IndexBufferDescExt
{
	ID3D12Resource* resource{ nullptr };
	uint64_t gpuAddress{ 0 };
	D3D12_INDEX_BUFFER_VIEW ibvHandle;

	constexpr IndexBufferDescExt& SetResource(ID3D12Resource* value) noexcept { resource = value; return *this; }
	constexpr IndexBufferDescExt& SetGpuAddress(uint64_t value) noexcept { gpuAddress = value; return *this; }
	constexpr IndexBufferDescExt& SetIbvHandle(D3D12_INDEX_BUFFER_VIEW value) noexcept { ibvHandle = value; return *this; }
};


class __declspec(uuid("9138A2AC-37DF-4B04-91B7-293B8134CCCF")) IGpuBufferData : public IGpuResourceData
{
public:
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const noexcept = 0;
	virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUAV() const noexcept = 0;
};


class __declspec(uuid("1066924B-D810-4FB8-927A-EC806F6F945D")) IIndexBufferData : public IGpuBufferData
{
public:
	virtual D3D12_INDEX_BUFFER_VIEW GetIBV() const noexcept = 0;
};


class __declspec(uuid("7BCB7DD3-EDEE-4CBD-9C4B-3D32159EE496")) IndexBufferData final
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IIndexBufferData, IGpuBufferData, IGpuResourceData, IPlatformData>>
	, NonCopyable
{
public:
	explicit IndexBufferData(const IndexBufferDescExt& descExt);

	ID3D12Resource* GetResource() const noexcept override { return m_resource.get(); }
	uint64_t GetGpuAddress() const noexcept override { return m_gpuAddress; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const noexcept override { return m_srvHandle; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetUAV() const noexcept override { return m_uavHandle; }
	D3D12_INDEX_BUFFER_VIEW GetIBV() const noexcept override { return m_ibvHandle; }

protected:
	wil::com_ptr<ID3D12Resource> m_resource;
	D3D12_CPU_DESCRIPTOR_HANDLE m_srvHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE m_uavHandle;
	D3D12_INDEX_BUFFER_VIEW m_ibvHandle;

	uint64_t m_gpuAddress{ 0 };
};

} // namespace Luna::DX12
