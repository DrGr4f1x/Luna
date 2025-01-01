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

#include "DepthBuffer.h"

#include "GraphicsCommon.h"


namespace Luna
{

bool DepthBuffer::Initialize(DepthBufferDesc& desc)
{
	m_bIsInitialized = true;
	return m_bIsInitialized;
}


void DepthBuffer::Reset()
{
	m_bIsInitialized = false;
	m_clearDepth = 1.0f;
	m_clearStencil = 0;

	PixelBuffer::Reset();
}

} // namespace Luna