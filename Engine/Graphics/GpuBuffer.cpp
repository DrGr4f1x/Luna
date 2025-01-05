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

#include "GpuBuffer.h"

#include "GraphicsCommon.h"


namespace Luna
{

void GpuBuffer::Reset()
{
	m_elementCount = 0;
	m_elementSize = 0;
}


bool GpuBuffer::Initialize(GpuBufferDesc& desc)
{
	// Validate description
	assert(IsBufferType(desc.resourceType));
	assert(desc.memoryAccess != MemoryAccess::Unknown);
	assert(desc.elementCount > 0);
	assert(desc.elementSize > 0);

	Reset();
	
	if (auto device = GetGraphicsDevice())
	{
		desc.SetResourceType(m_resourceType);

		m_platformData = device->CreateGpuBufferData(desc, m_usageState);

		m_name = desc.name;
		m_transitioningState = ResourceState::Undefined;

		m_elementCount = desc.elementCount;
		m_elementSize = desc.elementSize;
	}

	m_bIsInitialized = m_platformData != nullptr;

	// TODO
	/*if (IsInitialized() && desc.initialData)
	{
		CommandContext::InitializeBuffer(*this, desc.initialData, GetSize());
	}*/

	return m_bIsInitialized;
}

} // namespace Luna