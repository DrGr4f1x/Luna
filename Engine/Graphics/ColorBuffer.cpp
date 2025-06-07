//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#include "StdAfx.h"

#include "ColorBuffer.h"

#include "GraphicsCommon.h"

#include "ResourceManager.h"


namespace Luna
{

void ColorBuffer::Initialize(const ColorBufferDesc& ColorBufferDesc)
{
	m_handle = GetResourceManager()->CreateColorBuffer(ColorBufferDesc);
}


Color ColorBuffer::GetClearColor() const
{
	auto res = GetResourceManager()->GetClearColor(m_handle.get());
	assert(res.has_value());
	return *res;
}


void ColorBuffer::SetHandle(ResourceHandle handle) 
{ 
	m_handle = handle; 
}

} // namespace Luna