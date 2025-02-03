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
#include "Graphics\DX12\DirectXCommon.h"

using namespace Microsoft::WRL;


namespace Luna::DX12
{

class __declspec(uuid("D673302E-0617-4432-96C4-30A4F4E30195")) DescriptorSet final
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, IDescriptorSet>
	, NonCopyable
{
	enum { MaxDescriptors = 32 };

public:
	DescriptorSet();

	void SetSRV(int paramIndex, const IColorBuffer* colorBuffer) override;
	void SetSRV(int paramIndex, const IDepthBuffer* depthBuffer, bool depthSrv = true) override;
	void SetSRV(int paramIndex, const IGpuBuffer* gpuBuffer) override;

	void SetUAV(int paramIndex, const IColorBuffer* colorBuffer, uint32_t uavIndex = 0) override;
	void SetUAV(int paramIndex, const IDepthBuffer* depthBuffer) override;
	void SetUAV(int paramIndex, const IGpuBuffer* gpuBuffer) override;

	void SetCBV(int paramIndex, const IGpuBuffer* gpuBuffer) override;

	void SetDynamicOffset(uint32_t offset) override;

private:
	bool IsDirty() const noexcept { return m_dirtyBits != 0; }
	void Update();
	void SetDescriptor(int paramIndex, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);

private:
	std::array<D3D12_CPU_DESCRIPTOR_HANDLE, MaxDescriptors> m_descriptors;
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuDescriptor{ D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN };
	uint64_t m_gpuAddress{ D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN };
	uint32_t m_dirtyBits{ 0 };
	uint32_t m_dynamicOffset{ 0 };

	bool m_bIsSamplerTable{ false };
	bool m_bIsRootCBV{ false };
};

} // namespace Luna::DX12