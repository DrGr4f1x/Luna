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

#include "Texture12.h"

namespace Luna::DX12
{

Texture::Texture(const TextureDesc& desc, Device* device, ID3D12Resource* resource, const std::string& name)
	: ITexture{ desc }
	, m_resource{ resource }
	, m_name{ name }
{
	m_srvDescriptor.SetDevice(device);
}

} // namespace Luna::DX12