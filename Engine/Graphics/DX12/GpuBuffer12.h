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