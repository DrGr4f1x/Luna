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

#include "RootSignaturePool12.h"

#include "DescriptorSetPool12.h"


namespace Luna::DX12
{

RootSignaturePool* g_rootSignaturePool{ nullptr };


RootSignaturePool::RootSignaturePool(ID3D12Device* device)
	: m_device{ device }
{
	assert(g_rootSignaturePool == nullptr);

	// Populate free list and data arrays
	for (uint32_t i = 0; i < MaxItems; ++i)
	{
		m_freeList.push(i);
		m_rootSignatureData[i] = RootSignatureData{};
		m_descs[i] = RootSignatureDesc{};
	}

	g_rootSignaturePool = this;
}


RootSignaturePool::~RootSignaturePool()
{
	g_rootSignaturePool = nullptr;
}


RootSignatureHandle RootSignaturePool::CreateRootSignature(const RootSignatureDesc& rootSignatureDesc)
{
	std::lock_guard guard(m_allocationMutex);

	assert(!m_freeList.empty());

	uint32_t index = m_freeList.front();
	m_freeList.pop();

	m_descs[index] = rootSignatureDesc;

	m_rootSignatureData[index] = FindOrCreateRootSignatureData(rootSignatureDesc);

	return Create<RootSignatureHandleType>(index, this);
}


void RootSignaturePool::DestroyHandle(RootSignatureHandleType* handle)
{
	assert(handle != nullptr);

	std::lock_guard guard(m_allocationMutex);

	// TODO: queue this up to execute in one big batch, e.g. at the end of the frame

	uint32_t index = handle->GetIndex();
	m_descs[index] = RootSignatureDesc{};
	m_rootSignatureData[index] = RootSignatureData{};

	m_freeList.push(index);
}


const RootSignatureDesc& RootSignaturePool::GetDesc(RootSignatureHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_descs[index];
}


DescriptorSetHandle RootSignaturePool::CreateDescriptorSet(RootSignatureHandleType* handle, uint32_t index) const
{
	const auto& rootSignatureDesc = GetDesc(handle);

	assert(index < rootSignatureDesc.rootParameters.size());

	const auto& rootParam = rootSignatureDesc.rootParameters[index];

	const bool isRootBuffer = rootParam.parameterType == RootParameterType::RootCBV ||
		rootParam.parameterType == RootParameterType::RootSRV ||
		rootParam.parameterType == RootParameterType::RootUAV;

	const bool isSamplerTable = rootParam.IsSamplerTable();

	const D3D12_DESCRIPTOR_HEAP_TYPE heapType = isSamplerTable
		? D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER
		: D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	DescriptorSetDesc descriptorSetDesc{
		.descriptorHandle	= isRootBuffer ? DescriptorHandle{} : AllocateUserDescriptor(heapType),
		.numDescriptors		= rootParam.GetNumDescriptors(),
		.isSamplerTable		= isSamplerTable,
		.isRootBuffer		= isRootBuffer
	};

	return GetD3D12DescriptorSetPool()->CreateDescriptorSet(descriptorSetDesc);
}


ID3D12RootSignature* RootSignaturePool::GetRootSignature(RootSignatureHandleType* handle) const
{
	assert(handle != nullptr);

	uint32_t index = handle->GetIndex();
	return m_rootSignatureData[index].rootSignature.get();
}


RootSignatureData RootSignaturePool::FindOrCreateRootSignatureData(const RootSignatureDesc& rootSignatureDesc)
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
		return RootSignatureData{};
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
		.Version = D3D_ROOT_SIGNATURE_VERSION_1_1,
		.Desc_1_1 = {
			.NumParameters = (uint32_t)d3d12RootParameters.size(),
			.pParameters = d3d12RootParameters.data(),
			.NumStaticSamplers = 0,
			.pStaticSamplers = nullptr,
			.Flags = RootSignatureFlagsToDX12(rootSignatureDesc.flags)
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

	ID3D12RootSignature** ppRootSignature{ nullptr };
	ID3D12RootSignature* pRootSignature{ nullptr };
	bool firstCompile = false;
	{
		lock_guard<mutex> CS(m_rootSignatureMutex);
		auto iter = m_rootSignatureHashMap.find(hashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == m_rootSignatureHashMap.end())
		{
			ppRootSignature = m_rootSignatureHashMap[hashCode].addressof();
			firstCompile = true;
		}
		else
		{
			ppRootSignature = iter->second.addressof();
		}
	}

	if (firstCompile)
	{
		wil::com_ptr<ID3DBlob> pOutBlob, pErrorBlob;

		assert_succeeded(D3D12SerializeVersionedRootSignature(&d3d12RootSignatureDesc, &pOutBlob, &pErrorBlob));

		assert_succeeded(m_device->CreateRootSignature(0, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(),
			IID_PPV_ARGS(&pRootSignature)));

		SetDebugName(pRootSignature, rootSignatureDesc.name);

		m_rootSignatureHashMap[hashCode].attach(pRootSignature);
		assert(*ppRootSignature == pRootSignature);
	}
	else
	{
		while (*ppRootSignature == nullptr)
		{
			this_thread::yield();
		}
		pRootSignature = *ppRootSignature;
	}

	RootSignatureData rootSignatureData{
		.rootSignature = pRootSignature,
		.descriptorTableBitmap = descriptorTableBitmap,
		.samplerTableBitmap = samplerTableBitmap,
		.descriptorTableSizes = descriptorTableSize
	};

	return rootSignatureData;
}


RootSignaturePool* const GetD3D12RootSignaturePool()
{
	return g_rootSignaturePool;
}

} // namespace Luna::DX12