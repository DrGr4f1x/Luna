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

struct RootSignatureData
{
	wil::com_ptr<ID3D12RootSignature> rootSignature;
	uint32_t descriptorTableBitmap{ 0 };
	uint32_t samplerTableBitmap{ 0 };
	std::vector<uint32_t> descriptorTableSizes;
};


class RootSignatureManager : public IRootSignatureManager
{
	static const uint32_t MaxItems = (1 << 12);

public:
	explicit RootSignatureManager(ID3D12Device* device);
	~RootSignatureManager();

	// Create/Destroy RootSignature
	RootSignatureHandle CreateRootSignature(const RootSignatureDesc& rootSignatureDesc) override;
	void DestroyHandle(RootSignatureHandleType* handle) override;

	// Platform agnostic functions
	const RootSignatureDesc& GetDesc(const RootSignatureHandleType* handle) const override;
	uint32_t GetNumRootParameters(const RootSignatureHandleType* handle) const override;
	wil::com_ptr<DescriptorSetHandleType> CreateDescriptorSet(RootSignatureHandleType* handle, uint32_t index) const;

	// Getters
	ID3D12RootSignature* GetRootSignature(const RootSignatureHandleType* handle) const;
	uint32_t GetDescriptorTableBitmap(const RootSignatureHandleType* handle) const;
	uint32_t GetSamplerTableBitmap(const RootSignatureHandleType* handle) const;
	const std::vector<uint32_t>& GetDescriptorTableSize(const RootSignatureHandleType* handle) const;

private:
	const RootSignatureData& GetData(const RootSignatureHandleType* handle) const;
	RootSignatureData FindOrCreateRootSignatureData(const RootSignatureDesc& rootSignatureDesc);

private:
	wil::com_ptr<ID3D12Device> m_device;

	// Allocation mutex
	std::mutex m_allocationMutex;

	// Free list
	std::queue<uint32_t> m_freeList;

	// Hot data: RootSignatureData
	std::array<RootSignatureData, MaxItems> m_rootSignatureData;

	// Cold data: RootSignatureDesc
	std::array<RootSignatureDesc, MaxItems> m_descs;

	// Root signatures
	std::mutex m_rootSignatureMutex;
	std::map<size_t, wil::com_ptr<ID3D12RootSignature>> m_rootSignatureHashMap;
};


RootSignatureManager* const GetD3D12RootSignatureManager();

} // namespace Luna::DX12