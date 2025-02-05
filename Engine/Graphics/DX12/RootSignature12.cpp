//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#include "Stdafx.h"

#include "RootSignature12.h"

#include "DescriptorSet12.h"
#include "Device12.h"
#include "DirectXCommon.h"


namespace Luna::DX12
{

RootSignature12::RootSignature12(const RootSignatureDesc& rootSignatureDesc, const RootSignatureDescExt& rootSignatureDescExt)
	: m_desc{ rootSignatureDesc }
	, m_rootSignature{ rootSignatureDescExt.rootSignature }
	, m_descriptorTableBitmap{ rootSignatureDescExt.descriptorTableBitmap }
	, m_samplerTableBitmap{ rootSignatureDescExt.samplerTableBitmap }
	, m_descriptorTableSize{ rootSignatureDescExt.descriptorTableSize }
{}


NativeObjectPtr RootSignature12::GetNativeObject(NativeObjectType type) const noexcept
{
	using enum NativeObjectType;

	switch (type)
	{
	case DX12_RootSignature:
		return NativeObjectPtr(GetRootSignature());

	default:
		assert(false);
		return nullptr;
	}
}


uint32_t RootSignature12::GetNumRootParameters() const noexcept
{
	return (uint32_t)m_desc.rootParameters.size();
}


const RootParameter& RootSignature12::GetRootParameter(uint32_t index) const noexcept
{
	return m_desc.rootParameters[index];
}


DescriptorSetHandle RootSignature12::CreateDescriptorSet(uint32_t index) const
{
	assert(index < m_desc.rootParameters.size());

	const auto& rootParam = GetRootParameter(index);
	bool isSamplerTable = rootParam.IsSamplerTable();
	
	const D3D12_DESCRIPTOR_HEAP_TYPE heapType = isSamplerTable
		? D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
		: D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	DescriptorHandle descriptorHandle = AllocateUserDescriptor(heapType);

	DescriptorSetDescExt descriptorSetDescExt{
		.descriptorHandle	= descriptorHandle,
		.numDescriptors		= rootParam.GetNumDescriptors(),
		.isSamplerTable		= isSamplerTable,
		.isRootCbv			= rootParam.parameterType == RootParameterType::RootCBV
	};

	return Make<DescriptorSet>(descriptorSetDescExt);
}

} // namespace Luna::DX12