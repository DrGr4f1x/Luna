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

#include "Graphics\DX12\DirectXCommon.h"

namespace Luna 
{

// Forward declarations
struct RootParameter;

} // namespace Luna;


namespace Luna::DX12
{

class DescriptorOffsetTable
{
public:
	DescriptorOffsetTable() = default;
	DescriptorOffsetTable(const RootParameter& rootParameter);

	uint32_t GetDescriptorOffset(DescriptorRegisterType type, uint32_t descriptorRegister) const;

	uint32_t GetSrvDescriptorOffset(uint32_t descriptorRegister) const;
	uint32_t GetUavDescriptorOffset(uint32_t descriptorRegister) const;
	uint32_t GetCbvDescriptorOffset(uint32_t descriptorRegister) const;
	uint32_t GetSamplerDescriptorOffset(uint32_t descriptorRegister) const;

private:
	void SetDescriptorOffset(DescriptorRegisterType type, uint32_t offset, uint32_t descriptorRegister);

private:
	// TODO: Optimize this for the case of fewer than 32 max registers
	std::unordered_map<uint32_t, uint32_t> m_srvOffsets;
	std::unordered_map<uint32_t, uint32_t> m_uavOffsets;
	std::unordered_map<uint32_t, uint32_t> m_cbvOffsets;
	std::unordered_map<uint32_t, uint32_t> m_samplerOffsets;
};

} // namespace Luna::DX12