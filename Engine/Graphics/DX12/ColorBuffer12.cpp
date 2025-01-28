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

#include "ColorBuffer12.h"


namespace Luna::DX12
{

ColorBufferData::ColorBufferData(const ColorBufferDescExt& descExt)
	: m_resource{ descExt.resource }
	, m_srvHandle{ descExt.srvHandle }
	, m_rtvHandle{ descExt.rtvHandle }
	, m_uavHandles{ descExt.uavHandles }
{
}


ColorBuffer12::ColorBuffer12(const ColorBufferDesc& desc, const ColorBufferDescExt& descExt)
	: m_name{ desc.name }
	, m_resourceType{ desc.resourceType }
	, m_usageState{ descExt.usageState }
	, m_width{ desc.width }
	, m_height{ desc.height }
	, m_arraySizeOrDepth{ desc.arraySizeOrDepth }
	, m_numMips{ desc.numMips }
	, m_numSamples{ desc.numSamples }
	, m_planeCount{ descExt.planeCount }
	, m_format{ desc.format }
	, m_clearColor{ desc.clearColor }
	, m_resource{ descExt.resource }
	, m_srvHandle{ descExt.srvHandle }
	, m_rtvHandle{ descExt.rtvHandle }
	, m_uavHandles{ descExt.uavHandles }
{}


NativeObjectPtr ColorBuffer12::GetNativeObject(NativeObjectType type, uint32_t index) const noexcept
{
	switch (type)
	{
	case NativeObjectType::DX12_Resource:
		return NativeObjectPtr(m_resource.get());

	case NativeObjectType::DX12_RTV:
		return NativeObjectPtr(m_rtvHandle.ptr);

	case NativeObjectType::DX12_SRV:
		return NativeObjectPtr(m_srvHandle.ptr);

	case NativeObjectType::DX12_UAV:
		return NativeObjectPtr(m_uavHandles[index].ptr);

	case NativeObjectType::DX12_GpuVirtualAddress:
		return NativeObjectPtr(m_resource->GetGPUVirtualAddress());

	default:
		assert(false);
		return nullptr;
	}
}


TextureDimension ColorBuffer12::GetDimension() const noexcept
{
	return ResourceTypeToTextureDimension(m_resourceType);
}

} // namespace Luna::DX12