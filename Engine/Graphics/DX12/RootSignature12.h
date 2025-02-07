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


class __declspec(uuid("401C875A-76F0-4F33-B300-FC55DBA83DD7")) IRootSignature12 : public IRootSignature
{
public:
	virtual ID3D12RootSignature* GetRootSignature() const noexcept = 0;
	virtual uint32_t GetDescriptorTableBitmap() const noexcept = 0;
	virtual uint32_t GetSamplerTableBitmap() const noexcept = 0;
	virtual const std::vector<uint32_t>& GetDescriptorTableSize() const noexcept = 0;
};


class __declspec(uuid("A23FFC17-0115-4437-BF81-F192185E7A89")) RootSignature12 final
	: public RuntimeClass<RuntimeClassFlags<ClassicCom>, ChainInterfaces<IRootSignature12, IRootSignature>>
	, NonCopyable
{
public:
	RootSignature12(const RootSignatureDesc& rootSignatureDesc, const RootSignatureDescExt& rootSignatureDescExt);

	// IRootSignature implementation
	NativeObjectPtr GetNativeObject(NativeObjectType type) const noexcept override;

	uint32_t GetNumRootParameters() const noexcept override;
	RootParameter& GetRootParameter(uint32_t index) noexcept override;
	const RootParameter& GetRootParameter(uint32_t index) const noexcept override;

	wil::com_ptr<IDescriptorSet> CreateDescriptorSet(uint32_t index) const override;
	wil::com_ptr<IResourceSet> CreateResourceSet() const override;

	// IRootSignature12 implementation
	ID3D12RootSignature* GetRootSignature() const noexcept override { return m_rootSignature.get(); }
	uint32_t GetDescriptorTableBitmap() const noexcept override { return m_descriptorTableBitmap; }
	uint32_t GetSamplerTableBitmap() const noexcept override { return m_samplerTableBitmap; }
	const std::vector<uint32_t>& GetDescriptorTableSize() const noexcept override { return m_descriptorTableSize; }

private:
	RootSignatureDesc m_desc;
	wil::com_ptr<ID3D12RootSignature> m_rootSignature;
	uint32_t m_descriptorTableBitmap{ 0 };
	uint32_t m_samplerTableBitmap{ 0 };
	std::vector<uint32_t> m_descriptorTableSize;
};

} // namespace Luna::DX12