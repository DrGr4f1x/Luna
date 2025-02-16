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

#include "Graphics\ColorBuffer.h"
#include "Graphics\DX12\DirectXCommon.h"

namespace Luna::DX12
{

struct ColorBufferData
{
	wil::com_ptr<ID3D12Resource> resource;
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 12> uavHandles;
	uint32_t planeCount{ 1 };
	ResourceState usageState{ ResourceState::Undefined };
};


class ColorBufferPool : public IColorBufferPool
{
	static const uint32_t MaxItems = (1 << 8);

public:
	ColorBufferPool(ID3D12Device* device, D3D12MA::Allocator* allocator);
	~ColorBufferPool();

	// Create/Destroy ColorBuffer
	ColorBufferHandle CreateColorBuffer(const ColorBufferDesc& colorBufferDesc) override;
	void DestroyHandle(ColorBufferHandleType* handle) override;

	// Platform agnostic functions
	ResourceType GetResourceType(ColorBufferHandleType* handle) const override;
	ResourceState GetUsageState(ColorBufferHandleType* handle) const override;
	void SetUsageState(ColorBufferHandleType* handle, ResourceState newState) override;
	uint64_t GetWidth(ColorBufferHandleType* handle) const override;
	uint32_t GetHeight(ColorBufferHandleType* handle) const override;
	uint32_t GetDepthOrArraySize(ColorBufferHandleType* handle) const override;
	uint32_t GetNumMips(ColorBufferHandleType* handle) const override;
	uint32_t GetNumSamples(ColorBufferHandleType* handle) const override;
	uint32_t GetPlaneCount(ColorBufferHandleType* handle) const override;
	Format GetFormat(ColorBufferHandleType* handle) const override;
	Color GetClearColor(ColorBufferHandleType* handle) const override;

	// Platform specific functions
	ColorBufferHandle CreateColorBufferFromSwapChain(IDXGISwapChain* swapChain, uint32_t imageIndex);
	ID3D12Resource* GetResource(ColorBufferHandleType* handle) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRV(ColorBufferHandleType* handle) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetRTV(ColorBufferHandleType* handle) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetUAV(ColorBufferHandleType* handle, uint32_t uavIndex) const;

private:
	const ColorBufferDesc& GetDesc(ColorBufferHandleType* handle) const;
	const ColorBufferData& GetData(ColorBufferHandleType* handle) const;
	ColorBufferData CreateColorBuffer_Internal(const ColorBufferDesc& colorBufferDesc);

private:
	wil::com_ptr<ID3D12Device> m_device;
	wil::com_ptr<D3D12MA::Allocator> m_allocator;

	// Allocation mutex
	std::mutex m_allocationMutex;

	// Free list
	std::queue<uint32_t> m_freeList;

	// Cold data
	std::array<ColorBufferDesc, MaxItems> m_descs;

	// Hot data
	std::array<ColorBufferData, MaxItems> m_colorBufferData;
};


ColorBufferPool* const GetD3D12ColorBufferPool();

} // namespace Luna::DX12