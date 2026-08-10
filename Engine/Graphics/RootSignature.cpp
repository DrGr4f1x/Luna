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

DescriptorOffsetTable::DescriptorOffsetTable(const RootParameter& rootParameter)
{
	if (rootParameter.parameterType == RootParameterType::Table)
	{
		uint32_t offset = 0;
		for (const auto& range : rootParameter.table)
		{
			uint32_t startRegister = range.startRegister;
			assert(startRegister != APPEND_REGISTER);

			DescriptorRegisterType registerType = GetDescriptorRegisterType(range.descriptorType);

			for (uint32_t i = 0; i < range.numDescriptors; ++i)
			{
				SetDescriptorOffset(registerType, offset, startRegister + i);

				++offset;
			}
		}
	}
}


uint32_t DescriptorOffsetTable::GetDescriptorOffset(DescriptorRegisterType type, uint32_t descriptorRegister) const
{
	switch (type)
	{
	case DescriptorRegisterType::CBV:
	{
		auto it = m_cbvOffsets.find(descriptorRegister);
		assert(it != m_cbvOffsets.end());
		return it->second;
	}
	break;

	case DescriptorRegisterType::SRV:
	{
		auto it = m_srvOffsets.find(descriptorRegister);
		assert(it != m_srvOffsets.end());
		return it->second;
	}
	break;

	case DescriptorRegisterType::UAV:
	{
		auto it = m_uavOffsets.find(descriptorRegister);
		assert(it != m_uavOffsets.end());
		return it->second;
	}
	break;

	case DescriptorRegisterType::Sampler:
	{
		auto it = m_samplerOffsets.find(descriptorRegister);
		assert(it != m_samplerOffsets.end());
		return it->second;
	}
	break;
	}

	assert(false);
	return (uint32_t)-1;
}


void DescriptorOffsetTable::SetDescriptorOffset(DescriptorRegisterType type, uint32_t offset, uint32_t descriptorRegister)
{
	switch (type)
	{
	case DescriptorRegisterType::CBV:
		m_cbvOffsets[descriptorRegister] = offset;
		break;

	case DescriptorRegisterType::SRV:
		m_srvOffsets[descriptorRegister] = offset;
		break;

	case DescriptorRegisterType::UAV:
		m_uavOffsets[descriptorRegister] = offset;
		break;

	case DescriptorRegisterType::Sampler:
		m_samplerOffsets[descriptorRegister] = offset;
		break;
	}
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