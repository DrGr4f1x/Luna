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

#include "RootSignatureFactory12.h"

#include "Graphics\ResourceManager.h"


namespace Luna::DX12
{

RootSignatureFactory::RootSignatureFactory(IResourceManager* owner, ID3D12Device* device)
	: m_owner{ owner }
	, m_device{ device }
{
	for (uint32_t i = 0; i < MaxResources; ++i)
	{
		ResetDesc(i);
		ResetData(i);
		ResetHash(i);
		m_freeList.push(i);
	}
}


ResourceHandle RootSignatureFactory::CreateRootSignature(const RootSignatureDesc& rootSignatureDesc)
{
	std::vector<D3D12_ROOT_PARAMETER1> d3d12RootParameters;

	auto exitGuard = wil::scope_exit([&]()
		{
			for (auto& param : d3d12RootParameters)
			{
				if (param.DescriptorTable.NumDescriptorRanges > 0)
				{
					delete[] param.DescriptorTable.pDescriptorRanges;
				}
			}
		});

	// Validate RootSignatureDesc
	if (!rootSignatureDesc.Validate())
	{
		LogError(LogDirectX) << "RootSignature is not valid!" << endl;
		return nullptr;
	}

	// Build DX12 root parameter descriptions
	for (const auto& rootParameter : rootSignatureDesc.rootParameters)
	{
		if (rootParameter.parameterType == RootParameterType::RootConstants)
		{
			D3D12_ROOT_PARAMETER1& param = d3d12RootParameters.emplace_back();
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
			param.ShaderVisibility = ShaderStageToDX12(rootParameter.shaderVisibility);
			param.Constants.Num32BitValues = rootParameter.num32BitConstants;
			param.Constants.RegisterSpace = rootParameter.registerSpace;
			param.Constants.ShaderRegister = rootParameter.startRegister;
		}
		else if (rootParameter.parameterType == RootParameterType::RootCBV ||
			rootParameter.parameterType == RootParameterType::RootSRV ||
			rootParameter.parameterType == RootParameterType::RootUAV)
		{
			D3D12_ROOT_PARAMETER1& param = d3d12RootParameters.emplace_back();
			param.ParameterType = RootParameterTypeToDX12(rootParameter.parameterType);
			param.ShaderVisibility = ShaderStageToDX12(rootParameter.shaderVisibility);
			param.Descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC;
			param.Descriptor.RegisterSpace = rootParameter.registerSpace;
			param.Descriptor.ShaderRegister = rootParameter.startRegister;
		}
		else if (rootParameter.parameterType == RootParameterType::Table)
		{
			D3D12_ROOT_PARAMETER1& param = d3d12RootParameters.emplace_back();
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			param.ShaderVisibility = ShaderStageToDX12(rootParameter.shaderVisibility);

			const uint32_t numRanges = (uint32_t)rootParameter.table.size();
			param.DescriptorTable.NumDescriptorRanges = numRanges;
			D3D12_DESCRIPTOR_RANGE1* pRanges = new D3D12_DESCRIPTOR_RANGE1[numRanges];
			for (uint32_t i = 0; i < numRanges; ++i)
			{
				D3D12_DESCRIPTOR_RANGE1& d3d12Range = pRanges[i];
				const DescriptorRange& range = rootParameter.table[i];
				d3d12Range.RangeType = DescriptorTypeToDX12(range.descriptorType);
				d3d12Range.NumDescriptors = range.numDescriptors;
				d3d12Range.BaseShaderRegister = range.startRegister;
				d3d12Range.RegisterSpace = rootParameter.registerSpace;
				d3d12Range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
			}
			param.DescriptorTable.pDescriptorRanges = pRanges;
		}
	}

	D3D12_VERSIONED_ROOT_SIGNATURE_DESC d3d12RootSignatureDesc{
		.Version	= D3D_ROOT_SIGNATURE_VERSION_1_1,
		.Desc_1_1	= {
			.NumParameters		= (uint32_t)d3d12RootParameters.size(),
			.pParameters		= d3d12RootParameters.data(),
			.NumStaticSamplers	= 0,
			.pStaticSamplers	= nullptr,
			.Flags				= RootSignatureFlagsToDX12(rootSignatureDesc.flags)
		}
	};

	uint32_t descriptorTableBitmap{ 0 };
	uint32_t samplerTableBitmap{ 0 };
	std::vector<uint32_t> descriptorTableSize;
	descriptorTableSize.reserve(16);

	// Calculate hash
	size_t hashCode = Utility::HashState(&d3d12RootSignatureDesc.Version);
	hashCode = Utility::HashState(&d3d12RootSignatureDesc.Desc_1_1.Flags, 1, hashCode);

	for (uint32_t param = 0; param < d3d12RootSignatureDesc.Desc_1_1.NumParameters; ++param)
	{
		const D3D12_ROOT_PARAMETER1& rootParam = d3d12RootSignatureDesc.Desc_1_1.pParameters[param];
		descriptorTableSize.push_back(0);

		if (rootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
		{
			assert(rootParam.DescriptorTable.pDescriptorRanges != nullptr);

			hashCode = Utility::HashState(rootParam.DescriptorTable.pDescriptorRanges,
				rootParam.DescriptorTable.NumDescriptorRanges, hashCode);

			// We keep track of sampler descriptor tables separately from CBV_SRV_UAV descriptor tables
			if (rootParam.DescriptorTable.pDescriptorRanges->RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
			{
				samplerTableBitmap |= (1 << param);
			}
			else
			{
				descriptorTableBitmap |= (1 << param);
			}

			for (uint32_t tableRange = 0; tableRange < rootParam.DescriptorTable.NumDescriptorRanges; ++tableRange)
			{
				descriptorTableSize[param] += rootParam.DescriptorTable.pDescriptorRanges[tableRange].NumDescriptors;
			}
		}
		else
		{
			hashCode = Utility::HashState(&rootParam, 1, hashCode);
		}
	}

	RootSignatureRecord* record = nullptr;
	ResourceHandle returnHandle;
	bool firstCompile = false;
	{
		lock_guard<mutex> lock(m_mutex);

		auto iter = m_hashToRecordMap.find(hashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == m_hashToRecordMap.end())
		{
			firstCompile = true;

			auto recordPtr = make_unique<RootSignatureRecord>();
			m_hashToRecordMap.emplace(make_pair(hashCode, std::move(recordPtr)));

			record = m_hashToRecordMap[hashCode].get();
		}
		else
		{
			record = iter->second.get();
			returnHandle = record->weakHandle.lock();

			if (!returnHandle)
			{
				returnHandle.reset();
				record->isReady = false;
				firstCompile = true;
			}
		}
	}

	if (firstCompile)
	{
		wil::com_ptr<ID3DBlob> pOutBlob, pErrorBlob;

		assert_succeeded(D3D12SerializeVersionedRootSignature(&d3d12RootSignatureDesc, &pOutBlob, &pErrorBlob));

		wil::com_ptr<ID3D12RootSignature> pRootSignature;
		assert_succeeded(m_device->CreateRootSignature(0, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(),
			IID_PPV_ARGS(&pRootSignature)));

		SetDebugName(pRootSignature.get(), rootSignatureDesc.name);

		assert(!m_freeList.empty());

		uint32_t index = m_freeList.front();
		m_freeList.pop();

		m_descs[index] = rootSignatureDesc;
		
		RootSignatureData rootSignatureData{
			.rootSignature			= pRootSignature,
			.descriptorTableBitmap	= descriptorTableBitmap,
			.samplerTableBitmap		= samplerTableBitmap,
			.descriptorTableSizes	= descriptorTableSize
		};
		m_rootSignatureData[index] = rootSignatureData;
		m_hashList[index] = hashCode;

		returnHandle = make_shared<ResourceHandleType>(index, IResourceManager::ManagedRootSignature, m_owner);

		record->weakHandle = returnHandle;
		record->isReady = true;
	}
	else
	{
		while (!record->isReady)
		{
			this_thread::yield();
		}
	}

	assert(returnHandle);
	return returnHandle;
}


void RootSignatureFactory::Destroy(uint32_t index)
{
	std::lock_guard lock(m_mutex);

	size_t hash = m_hashList[index];

	m_hashToRecordMap.erase(hash);

	ResetDesc(index);
	ResetData(index);
	ResetHash(index);
	m_freeList.push(index);
}


const RootSignatureDesc& RootSignatureFactory::GetDesc(uint32_t index) const
{
	return m_descs[index];
}


ID3D12RootSignature* RootSignatureFactory::GetRootSignature(uint32_t index) const
{
	return m_rootSignatureData[index].rootSignature.get();
}


uint32_t RootSignatureFactory::GetDescriptorTableBitmap(uint32_t index) const
{
	return m_rootSignatureData[index].descriptorTableBitmap;
}


uint32_t RootSignatureFactory::GetSamplerTableBitmap(uint32_t index) const
{
	return m_rootSignatureData[index].samplerTableBitmap;
}


const std::vector<uint32_t>& RootSignatureFactory::GetDescriptorTableSizes(uint32_t index) const
{
	return m_rootSignatureData[index].descriptorTableSizes;
}

} // namespace Luna::DX12