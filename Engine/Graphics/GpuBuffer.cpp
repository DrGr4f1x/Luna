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

#include "CommandContext.h"
#include "GraphicsCommon.h"

using namespace std;


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
	if (desc.resourceType == ResourceType::Unknown)
	{
		desc.resourceType = m_resourceType;
	}
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
	if (IsInitialized() && desc.initialData)
	{
		CommandContext::InitializeBuffer(*this, desc.initialData, GetSize());
	}

	return m_bIsInitialized;
}


VertexBuffer::~VertexBuffer()
{
	LogInfo(LogGraphics) << "Destroying VertexBuffer" << endl;
}


IndexBuffer::~IndexBuffer()
{
	LogInfo(LogGraphics) << "Destroying IndexBuffer" << endl;
}


ConstantBuffer::~ConstantBuffer()
{
	LogInfo(LogGraphics) << "Destroying ConstantBuffer" << endl;
}


bool StructuredBuffer::Initialize(GpuBufferDesc& desc)
{
	GpuBufferDesc counterBufferDesc{
		.name			= std::format("{} Counter Buffer", desc.name),
		.elementCount	= 1,
		.elementSize	= 4
	};

	return GpuBuffer::Initialize(desc) && m_counterBuffer.Initialize(counterBufferDesc);
}

} // namespace Luna