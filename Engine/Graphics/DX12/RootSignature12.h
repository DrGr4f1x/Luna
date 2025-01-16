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

using namespace Microsoft::WRL;


namespace Luna::DX12
{

struct RootSignatureDescExt
{
	ID3D12RootSignature* rootSignature{ nullptr };
	uint32_t descriptorTableBitmap{ 0 };
	uint32_t samplerTableBitmap{ 0 };
	std::vector<uint32_t> descriptorTableSize;

	constexpr RootSignatureDescExt& SetRootSignature(ID3D12RootSignature* value) noexcept { rootSignature = value; return *this; }
	constexpr RootSignatureDescExt& SetDescriptorTableBitmap(uint32_t value) noexcept { descriptorTableBitmap = value; return *this; }
	constexpr RootSignatureDescExt& SetSamplerTableBitmap(uint32_t value) noexcept { samplerTableBitmap = value; return *this; }
	RootSignatureDescExt& SetDescriptorTableSize(const std::vector<uint32_t>& value) { descriptorTableSize = value; return *this; }
};


class __declspec(uuid("52A34683-8F31-4FD3-B2DD-EEE555074589")) IRootSignatureData : public IPlatformData
{
public:
	virtual ID3D12RootSignature* GetRootSignature() const noexcept = 0;
	virtual uint32_t GetDescriptorTableBitmap() const noexcept = 0;
	virtual uint32_t GetSamplerTableBitmap() const noexcept = 0;
	virtual const std::vector<uint32_t>& GetDescriptorTableSize() const noexcept = 0;
};


class __declspec(uuid("9CE032FD-F849-4C90-B583-E605421836EF")) RootSignatureData final
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IRootSignatureData, IPlatformData>>
	, NonCopyable
{
public:
	explicit RootSignatureData(const RootSignatureDescExt& descExt);

	ID3D12RootSignature* GetRootSignature() const noexcept override { return m_rootSignature.get(); }
	uint32_t GetDescriptorTableBitmap() const noexcept override { return m_descriptorTableBitmap; }
	uint32_t GetSamplerTableBitmap() const noexcept override { return m_samplerTableBitmap; }
	const std::vector<uint32_t>& GetDescriptorTableSize() const noexcept override { return m_descriptorTableSize; }

private:
	wil::com_ptr<ID3D12RootSignature> m_rootSignature;
	uint32_t m_descriptorTableBitmap{ 0 };
	uint32_t m_samplerTableBitmap{ 0 };
	std::vector<uint32_t> m_descriptorTableSize;
};

} // namespace Luna::DX12