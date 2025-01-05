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

#include "GpuBuffer12.h"

namespace Luna::DX12
{

GpuBufferData::GpuBufferData(const GpuBufferDesc& desc, const GpuBufferDescExt& descExt)
	: m_resource{descExt.resource}
	, m_srvHandle{descExt.srvHandle}
	, m_uavHandle{descExt.uavHandle}
	, m_cbvHandle{descExt.cbvHandle}
	, m_format{desc.format}
	, m_bufferSize{desc.elementCount * desc.elementSize}
	, m_elementCount{desc.elementCount}
	, m_elementSize{desc.elementSize}
	, m_gpuAddress{descExt.gpuAddress}
{}

} // namespace Luna::DX12