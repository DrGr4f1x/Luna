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

#include "Graphics\DescriptorSet.h"
#include "Graphics\DX12\DescriptorAllocator12.h"
#include "Graphics\DX12\DirectXCommon.h"

using namespace Microsoft::WRL;


namespace Luna::DX12
{

struct DescriptorSetDescExt
{
	DescriptorHandle descriptorHandle;
	uint32_t numDescriptors{ 0 };
	bool isSamplerTable{ false };
	bool isRootBuffer{ false };
};


class __declspec(uuid("1FF0DA5B-4F66-4BEF-92A0-1E611D9440AF")) IDescriptorSet12 : public IDescriptorSet
{
public:
	virtual bool IsDirty() const = 0;
	virtual bool IsRootBuffer() const = 0;
	virtual bool IsSamplerTable() const = 0;
	virtual bool HasDescriptors() const = 0;
	virtual D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptor() const = 0;
	virtual uint64_t GetGpuAddress() const = 0;
	virtual uint32_t GetDynamicOffset() const = 0;
	virtual void Update() = 0;
};


class __declspec(uuid("D673302E-0617-4432-96C4-30A4F4E30195")) DescriptorSet final
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IDescriptorSet12, IDescriptorSet>>
	, NonCopyable
{
public:
	explicit DescriptorSet(const DescriptorSetDescExt& descriptorSetDescExt);

	// IDescriptorSet implementation
	void SetSRV(int slot, const IColorBuffer* colorBuffer) override;
	void SetSRV(int slot, const IDepthBuffer* depthBuffer, bool depthSrv = true) override;
	void SetSRV(int slot, const IGpuBuffer* gpuBuffer) override;

	void SetUAV(int slot, const IColorBuffer* colorBuffer, uint32_t uavIndex = 0) override;
	void SetUAV(int slot, const IDepthBuffer* depthBuffer) override;
	void SetUAV(int slot, const IGpuBuffer* gpuBuffer) override;

	void SetCBV(int slot, const IGpuBuffer* gpuBuffer) override;

	void SetDynamicOffset(uint32_t offset) override;

	// IDescriptorSet12 implementation
	bool IsDirty() const override { return m_dirtyBits != 0; }
	bool IsRootBuffer() const override { return m_isRootBuffer; }
	bool IsSamplerTable() const override { return m_isSamplerTable; }
	bool HasDescriptors() const override { return m_numDescriptors > 0; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptor() const override { return m_descriptorHandle.GetGpuHandle(); }
	uint64_t GetGpuAddress() const override { return m_gpuAddress; }
	uint32_t GetDynamicOffset() const override { return m_dynamicOffset; }
	void Update() override;

private:
	void SetDescriptor(int slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);

private:
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, MaxDescriptorsPerTable> m_descriptors;
	DescriptorHandle m_descriptorHandle;
	uint64_t m_gpuAddress{ D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN };
	uint32_t m_numDescriptors{ 0 };
	uint32_t m_dirtyBits{ 0 };
	uint32_t m_dynamicOffset{ 0 };

	bool m_isSamplerTable{ false };
	bool m_isRootBuffer{ false };
};

} // namespace Luna::DX12