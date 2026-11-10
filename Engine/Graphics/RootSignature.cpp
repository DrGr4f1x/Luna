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

#include "RootSignature.h"


namespace Luna
{

DescriptorSetPtr IRootSignature::CreateDescriptorSet(DescriptorHeapPtr descriptorHeap, uint32_t rootParamIndex, ColorBufferPtr colorBuffer) const
{ 
	auto descriptorSet = CreateDescriptorSet(descriptorHeap, rootParamIndex);

	if (descriptorSet)
	{
		const auto& rootParameter = m_desc.rootParameters[rootParamIndex];
		assert(rootParameter.parameterType == RootParameterType::Table);

		assert(rootParameter.table.size() == 1);

		const auto& range = rootParameter.table[0];

		assert(range.numDescriptors == 1);
		
		const auto descriptorRegisterType = GetDescriptorRegisterType(range.descriptorType);
		assert(descriptorRegisterType == DescriptorRegisterType::SRV || descriptorRegisterType == DescriptorRegisterType::UAV);
		if (descriptorRegisterType == DescriptorRegisterType::SRV)
		{
			descriptorSet->SetSRV(range.startRegister, colorBuffer);
		}
		else if (descriptorRegisterType == DescriptorRegisterType::UAV)
		{
			descriptorSet->SetUAV(range.startRegister, colorBuffer);
		}
	}

	return descriptorSet;
}


DescriptorSetPtr IRootSignature::CreateDescriptorSet(DescriptorHeapPtr descriptorHeap, uint32_t rootParamIndex, DepthBufferPtr depthBuffer, bool depthDescriptor) const
{
	auto descriptorSet = CreateDescriptorSet(descriptorHeap, rootParamIndex);

	if (descriptorSet)
	{
		const auto& rootParameter = m_desc.rootParameters[rootParamIndex];
		assert(rootParameter.parameterType == RootParameterType::Table);

		assert(rootParameter.table.size() == 1);

		const auto& range = rootParameter.table[0];

		assert(range.numDescriptors == 1);

		const auto descriptorRegisterType = GetDescriptorRegisterType(range.descriptorType);
		assert(descriptorRegisterType == DescriptorRegisterType::SRV || descriptorRegisterType == DescriptorRegisterType::UAV);
		if (descriptorRegisterType == DescriptorRegisterType::SRV)
		{
			descriptorSet->SetSRV(range.startRegister, depthBuffer, depthDescriptor);
		}
		else if (descriptorRegisterType == DescriptorRegisterType::UAV)
		{
			descriptorSet->SetUAV(range.startRegister, depthBuffer);
		}
	}

	return descriptorSet;
}


DescriptorSetPtr IRootSignature::CreateDescriptorSet(DescriptorHeapPtr descriptorHeap, uint32_t rootParamIndex, GpuBufferPtr gpuBuffer) const
{
	auto descriptorSet = CreateDescriptorSet(descriptorHeap, rootParamIndex);

	if (descriptorSet)
	{
		const auto& rootParameter = m_desc.rootParameters[rootParamIndex];
		assert(rootParameter.parameterType == RootParameterType::Table);

		assert(rootParameter.table.size() == 1);

		const auto& range = rootParameter.table[0];

		assert(range.numDescriptors == 1);

		const auto descriptorRegisterType = GetDescriptorRegisterType(range.descriptorType);
		assert(descriptorRegisterType != DescriptorRegisterType::Sampler);
		if (descriptorRegisterType == DescriptorRegisterType::SRV)
		{
			descriptorSet->SetSRV(range.startRegister, gpuBuffer);
		}
		else if (descriptorRegisterType == DescriptorRegisterType::UAV)
		{
			descriptorSet->SetUAV(range.startRegister, gpuBuffer);
		}
		else if (descriptorRegisterType == DescriptorRegisterType::CBV)
		{
			descriptorSet->SetCBV(range.startRegister, gpuBuffer);
		}
	}

	return descriptorSet;
}


DescriptorSetPtr IRootSignature::CreateDescriptorSet(DescriptorHeapPtr descriptorHeap, uint32_t rootParamIndex, TexturePtr texture) const
{
	auto descriptorSet = CreateDescriptorSet(descriptorHeap, rootParamIndex);

	if (descriptorSet)
	{
		const auto& rootParameter = m_desc.rootParameters[rootParamIndex];
		assert(rootParameter.parameterType == RootParameterType::Table);

		assert(rootParameter.table.size() == 1);

		const auto& range = rootParameter.table[0];

		assert(range.numDescriptors == 1);

		const auto descriptorRegisterType = GetDescriptorRegisterType(range.descriptorType);
		assert(descriptorRegisterType == DescriptorRegisterType::SRV);

		descriptorSet->SetSRV(range.startRegister, texture);
	}

	return descriptorSet;
}


DescriptorSetPtr IRootSignature::CreateDescriptorSet(DescriptorHeapPtr descriptorHeap, uint32_t rootParamIndex, SamplerPtr sampler) const
{
	auto descriptorSet = CreateDescriptorSet(descriptorHeap, rootParamIndex);

	if (descriptorSet)
	{
		const auto& rootParameter = m_desc.rootParameters[rootParamIndex];
		assert(rootParameter.parameterType == RootParameterType::Table);

		assert(rootParameter.table.size() == 1);

		const auto& range = rootParameter.table[0];

		assert(range.numDescriptors == 1);

		const auto descriptorRegisterType = GetDescriptorRegisterType(range.descriptorType);
		assert(descriptorRegisterType == DescriptorRegisterType::Sampler);

		descriptorSet->SetSampler(range.startRegister, sampler);
	}

	return descriptorSet;
}


DescriptorSetPtr IRootSignature::CreateDescriptorSet(DescriptorHeapPtr descriptorHeap, uint32_t rootParamIndex, std::initializer_list<DescriptorProvider> descriptorProviders) const
{
	auto descriptorSet = CreateDescriptorSet(descriptorHeap, rootParamIndex);

	const auto& rootParameter = m_desc.rootParameters[rootParamIndex];
	auto descIt = rootParameter.cbegin();

	for (auto descriptorProvider : descriptorProviders)
	{ 
		if (descIt == rootParameter.cend())
			break;

		const DescriptorRegisterType registerType = GetDescriptorRegisterType((*descIt).type);
		const uint32_t slot = (*descIt).registerIndex;

		switch (registerType)
		{
		case DescriptorRegisterType::SRV:
		{
			std::visit(overloaded{
				[slot, descriptorSet](ColorBufferPtr colorBuffer) { descriptorSet->SetSRV(slot, colorBuffer); },
				[slot, descriptorSet](DepthBufferPtr depthBuffer) { descriptorSet->SetSRV(slot, depthBuffer, true);  },
				[slot, descriptorSet](StencilSrv stencilSrv) { descriptorSet->SetSRV(slot, stencilSrv.depthBuffer, false); },
				[slot, descriptorSet](GpuBufferPtr gpuBuffer) { descriptorSet->SetSRV(slot, gpuBuffer); },
				[slot, descriptorSet](TexturePtr texture) { descriptorSet->SetSRV(slot, texture); },
				[slot, descriptorSet](StencilUav stencilUav) { assert(false); }
				}, descriptorProvider);
		}
			break;

		case DescriptorRegisterType::UAV:
		{
			std::visit(overloaded{
				[slot, descriptorSet](ColorBufferPtr colorBuffer) { descriptorSet->SetUAV(slot, colorBuffer); },
				[slot, descriptorSet](DepthBufferPtr depthBuffer) { descriptorSet->SetUAV(slot, depthBuffer);  },
				[slot, descriptorSet](StencilSrv stencilSrv) { assert(false); },
				[slot, descriptorSet](GpuBufferPtr gpuBuffer) { descriptorSet->SetUAV(slot, gpuBuffer); },
				[slot, descriptorSet](TexturePtr texture) { assert(false); },
				[slot, descriptorSet](StencilUav stencilUav) { assert(false); }
				}, descriptorProvider);
		}
		break;

		case DescriptorRegisterType::CBV:
		{
			std::visit(overloaded{
				[slot, descriptorSet](GpuBufferPtr gpuBuffer) { descriptorSet->SetCBV(slot, gpuBuffer); },
				[slot, descriptorSet](auto&& other) { assert(false); }
				}, descriptorProvider);
		}
		break;

		case DescriptorRegisterType::Sampler:
			assert(false);
			break;
		}

		++descIt;
	}

	return descriptorSet;
}


DescriptorSetPtr IRootSignature::CreateDescriptorSet(DescriptorHeapPtr descriptorHeap, uint32_t rootParamIndex, std::initializer_list<SamplerPtr> samplerProviders) const
{
	auto descriptorSet = CreateDescriptorSet(descriptorHeap, rootParamIndex);

	const auto& rootParameter = m_desc.rootParameters[rootParamIndex];
	assert(rootParameter.IsSamplerTable());

	auto descIt = rootParameter.cbegin();

	for (auto sampler : samplerProviders)
	{
		if (descIt == rootParameter.cend())
			break;

		const DescriptorRegisterType registerType = GetDescriptorRegisterType((*descIt).type);
		const uint32_t slot = (*descIt).registerIndex;

		assert(registerType == DescriptorRegisterType::Sampler);
		descriptorSet->SetSampler(slot, sampler);

		++descIt;
	}

	return descriptorSet;
}


uint32_t IRootSignature::GetNumRootParameters() const noexcept
{
	return (uint32_t)m_desc.rootParameters.size();
}


const RootParameter& IRootSignature::GetRootParameter(uint32_t index) const noexcept
{
	assert(index < (uint32_t)m_desc.rootParameters.size());
	return m_desc.rootParameters[index];
}

} // using namespace Luna