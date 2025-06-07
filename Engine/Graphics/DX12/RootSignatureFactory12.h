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


namespace Luna
{

// Forward declarations
class IResourceManager;

} // namespace Luna


namespace Luna::DX12
{

struct RootSignatureRecord
{
	std::weak_ptr<ResourceHandleType> weakHandle;
	std::atomic<bool> isReady{ false };
};


struct RootSignatureData
{
	wil::com_ptr<ID3D12RootSignature> rootSignature;
	uint32_t descriptorTableBitmap{ 0 };
	uint32_t samplerTableBitmap{ 0 };
	std::vector<uint32_t> descriptorTableSizes;
};


class RootSignatureFactory : public RootSignatureFactoryBase
{
public:
	RootSignatureFactory(IResourceManager* owner, ID3D12Device* device);

	ResourceHandle CreateRootSignature(const RootSignatureDesc& rootSignatureDesc);
	void Destroy(uint32_t index);

	const RootSignatureDesc& GetDesc(uint32_t index) const;

	ID3D12RootSignature* GetRootSignature(uint32_t index) const;
	uint32_t GetDescriptorTableBitmap(uint32_t index) const;
	uint32_t GetSamplerTableBitmap(uint32_t index) const;
	const std::vector<uint32_t>& GetDescriptorTableSizes(uint32_t index) const;

private:
	void ResetData(uint32_t index)
	{
		m_rootSignatureData[index] = RootSignatureData{};
	}

	void ResetHash(uint32_t index)
	{
		m_hashList[index] = 0;
	}

private:
	IResourceManager* m_owner{ nullptr };
	wil::com_ptr<ID3D12Device> m_device;

	std::mutex m_mutex;

	std::queue<uint32_t> m_freeList;

	std::map<size_t, std::unique_ptr<RootSignatureRecord>> m_hashToRecordMap;

	// Root signatures
	std::array<RootSignatureData, MaxResources> m_rootSignatureData;

	// Hash keys
	std::array<size_t, MaxResources> m_hashList;
};

} // namespace Luna::DX12