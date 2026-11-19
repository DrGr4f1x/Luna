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

#include "Graphics\GpuBuffer.h"

namespace Luna
{

struct ShaderRecord
{
	uint32_t id{ 0 };
	void* data{ nullptr };
	size_t size{ 0 };
};


struct ShaderBindingTableDesc
{
	std::vector<ShaderRecord> rayGenShaders;
	std::vector<ShaderRecord> missShaders;
	std::vector<ShaderRecord> hitGroups;
};


struct SBTBuffer
{
	GpuBufferPtr buffer;
	uint64_t bufferAddress;
	size_t size{ 0 };
	size_t stride{ 0 };
};


class ShaderBindingTable
{
public:
	ShaderBindingTable(SBTBuffer rayGenBuffer, SBTBuffer missBuffer, SBTBuffer hitGroupBuffer, SBTBuffer callableBuffer)
		: m_rayGenBuffer{ rayGenBuffer }
		, m_missBuffer{ missBuffer }
		, m_hitGroupBuffer{ hitGroupBuffer }
		, m_callableBuffer{ callableBuffer }
	{}
	virtual ~ShaderBindingTable() = default;

	SBTBuffer GetRayGenBuffer() const { return m_rayGenBuffer; }
	SBTBuffer GetMissBuffer() const { return m_missBuffer; }
	SBTBuffer GetHitGroupBuffer() const { return m_hitGroupBuffer; }
	SBTBuffer GetCallableBuffer() const { return m_callableBuffer; }

protected:
	SBTBuffer m_rayGenBuffer;
	SBTBuffer m_missBuffer;
	SBTBuffer m_hitGroupBuffer;
	SBTBuffer m_callableBuffer;
};

using ShaderBindingTablePtr = std::shared_ptr<ShaderBindingTable>;

} // namespace Luna