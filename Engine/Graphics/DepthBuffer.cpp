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

#include "ResourceManager.h"


namespace Luna
{

void DepthBuffer::Initialize(const DepthBufferDesc& depthBufferDesc)
{
	m_handle = GetResourceManager()->CreateDepthBuffer(depthBufferDesc);
}


float DepthBuffer::GetClearDepth() const
{
	auto res = GetResourceManager()->GetClearDepth(m_handle.get());
	assert(res.has_value());
	return *res;
}


uint8_t DepthBuffer::GetClearStencil() const
{
	auto res = GetResourceManager()->GetClearStencil(m_handle.get());
	assert(res.has_value());
	return *res;
}

} // namespace Luna