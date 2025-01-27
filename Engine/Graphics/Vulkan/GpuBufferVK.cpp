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

#include "GpuBufferVK.h"


namespace Luna::VK
{

GpuBufferVK::GpuBufferVK(const GpuBufferDesc& gpuBufferDesc, const GpuBufferDescExt& gpuBufferDescExt)
	: m_name{ gpuBufferDesc.name }
	, m_resourceType{ gpuBufferDesc.resourceType }
	, m_usageState{ gpuBufferDescExt.usageState }
	, m_elementCount{ gpuBufferDesc.elementCount }
	, m_elementSize{ gpuBufferDesc.elementSize }
	, m_buffer{ gpuBufferDescExt.buffer }
	, m_bufferInfo{ gpuBufferDescExt.bufferInfo }
{}


NativeObjectPtr GpuBufferVK::GetNativeObject(NativeObjectType type, uint32_t index) const noexcept
{
	using enum NativeObjectType;

	switch (type)
	{
	case VK_Buffer:
		return NativeObjectPtr(m_buffer->Get());

	default:
		assert(false);
		return nullptr;
	}
}

} // namespace Luna::VK