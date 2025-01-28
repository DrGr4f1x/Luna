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

#include "Device12.h"


namespace Luna::DX12
{

GpuBufferData::GpuBufferData(const GpuBufferDesc& desc, const GpuBufferDescExt& descExt)
	: m_resource{ descExt.resource }
	, m_allocation{ descExt.allocation }
	, m_srvHandle{ descExt.srvHandle }
	, m_uavHandle{ descExt.uavHandle }
	, m_format{ desc.format }
	, m_bufferSize{ desc.elementCount * desc.elementSize }
	, m_elementCount{ desc.elementCount }
	, m_elementSize{ desc.elementSize }
{}


GpuBuffer12::GpuBuffer12(const GpuBufferDesc& gpuBufferDesc, const GpuBufferDescExt& gpuBufferDescExt)
	: m_name{ gpuBufferDesc.name }
	, m_resourceType{ gpuBufferDesc.resourceType }
	, m_usageState{ gpuBufferDescExt.usageState }
	, m_elementCount{ gpuBufferDesc.elementCount }
	, m_elementSize{ gpuBufferDesc.elementSize }
	, m_resource{ gpuBufferDescExt.resource }
	, m_allocation{ gpuBufferDescExt.allocation }
	, m_srvHandle{ gpuBufferDescExt.srvHandle }
	, m_uavHandle{ gpuBufferDescExt.uavHandle }
{}


NativeObjectPtr GpuBuffer12::GetNativeObject(NativeObjectType type, uint32_t index) const noexcept
{
	using enum NativeObjectType;

	switch (type)
	{
	case DX12_Resource:
		return NativeObjectPtr(m_resource.get());

	case DX12_SRV:
		return NativeObjectPtr(m_srvHandle.ptr);

	case DX12_UAV:
		return NativeObjectPtr(m_uavHandle.ptr);

	case DX12_GpuVirtualAddress:
		return NativeObjectPtr(m_resource->GetGPUVirtualAddress());

	default:
		assert(false);
		return nullptr;
	}
}

} // namespace Luna::DX12