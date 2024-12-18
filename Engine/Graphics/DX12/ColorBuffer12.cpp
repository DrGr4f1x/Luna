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

ColorBuffer::ColorBuffer(const ColorBufferDesc& desc, const ColorBufferDescExt& descExt)
	: m_name{ desc.name }
	, m_resource{ descExt.resource }
	, m_usageState{ descExt.usageState }
	, m_resourceType{ desc.resourceType }
	, m_width{ desc.width }
	, m_height{ desc.height }
	, m_arraySizeOrDepth{ desc.arraySizeOrDepth }
	, m_numMips{ desc.numMips }
	, m_numSamples{ desc.numSamples }
	, m_planeCount{ descExt.planeCount }
	, m_format{ desc.format }
	, m_clearColor{ desc.clearColor }
	, m_srvHandle{ descExt.srvHandle }
	, m_rtvHandle{ descExt.rtvHandle }
	, m_uavHandles{ descExt.uavHandles }
{
	m_numMips = m_numMips == 0 ? ComputeNumMips(m_width, m_height) : m_numMips;
}


NativeObjectPtr ColorBuffer::GetNativeObject(NativeObjectType nativeObjectType) const noexcept
{
	if (nativeObjectType == NativeObjectType::DX12_Resource)
	{
		return NativeObjectPtr(m_resource.Get());
	}

	return IGpuImage::GetNativeObject(nativeObjectType);
}

} // namespace Luna::DX12