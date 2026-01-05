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

#include "Graphics\RootSignature.h"
#include "Graphics\DX12\DirectXCommon.h"


namespace Luna::DX12
{

// Forward declarations
class Device;


class RootSignature : public IRootSignature
{
	friend class Device;

public:
	Luna::DescriptorSetPtr CreateDescriptorSet(uint32_t rootParamIndex) const override;

	ID3D12RootSignature* GetRootSignature() const noexcept { return m_rootSignature.get(); }
	uint32_t GetDescriptorTableBitmap() const noexcept { return m_descriptorTableBitmap; }
	uint32_t GetSamplerTableBitmap() const noexcept { return m_samplerTableBitmap; }
	const std::vector<uint32_t> GetDescriptorTableSizes() const noexcept { return m_descriptorTableSizes; }

protected:
	Device* m_device{ nullptr };

	wil::com_ptr<ID3D12RootSignature> m_rootSignature;
	uint32_t m_descriptorTableBitmap{ 0 };
	uint32_t m_samplerTableBitmap{ 0 };
	std::vector<uint32_t> m_descriptorTableSizes;
};

} // namespace Luna::DX12