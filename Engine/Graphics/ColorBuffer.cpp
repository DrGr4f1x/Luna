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

#include "ColorBuffer.h"

#include "GraphicsCommon.h"


namespace Luna
{

bool ColorBuffer::Initialize(ColorBufferDesc& desc)
{
	Reset();

	if (auto device = GetGraphicsDevice())
	{
		m_platformData = device->CreateColorBufferData(desc, m_usageState);

		m_name = desc.name;
		m_usageState = ResourceState::Undefined;
		m_transitioningState = ResourceState::Undefined;
		m_resourceType = desc.resourceType;

		m_width = desc.width;
		m_height = desc.height;
		m_arraySizeOrDepth = desc.arraySizeOrDepth;
		m_numMips = desc.numMips;
		m_numSamples = desc.numSamples;
		m_planeCount = desc.planeCount;
		m_format = desc.format;

		m_clearColor = desc.clearColor;

		m_numMips = m_numMips == 0 ? ComputeNumMips(m_width, m_height) : m_numMips;
	}

	m_bIsInitialized = m_platformData != nullptr;

	return m_bIsInitialized;
}


bool ColorBuffer::InitializeFromSwapchain(uint32_t imageIndex)
{
	auto deviceManager = GetDeviceManager();
	if (deviceManager)
	{
		ColorBufferDesc desc{};
		m_platformData = deviceManager->CreateColorBufferFromSwapChain(desc, m_usageState, imageIndex);

		m_name = desc.name;
		m_usageState = ResourceState::Undefined;
		m_transitioningState = ResourceState::Undefined;
		m_resourceType = desc.resourceType;

		m_width = desc.width;
		m_height = desc.height;
		m_arraySizeOrDepth = desc.arraySizeOrDepth;
		m_numMips = desc.numMips;
		m_numSamples = desc.numSamples;
		m_planeCount = desc.planeCount;
		m_format = desc.format;

		m_clearColor = desc.clearColor;

		m_numMips = m_numMips == 0 ? ComputeNumMips(m_width, m_height) : m_numMips;
	}

	m_bIsInitialized = m_platformData != nullptr;

	return m_bIsInitialized;
}


void ColorBuffer::Reset()
{
	m_bIsInitialized = false;
	m_clearColor = DirectX::Colors::Black;

	PixelBuffer::Reset();
}

} // namespace Luna