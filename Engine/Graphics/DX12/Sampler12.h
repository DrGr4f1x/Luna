//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#pragma once

#include "Graphics\Sampler.h"
#include "Graphics\DX12\Descriptor12.h"
#include "Graphics\DX12\DirectXCommon.h"


namespace Luna::DX12
{

// Forward declarations
class Device;


class Sampler : public ISampler
{
	friend class Device;
	
public:
	Sampler(Device* device);

	const Descriptor& GetDescriptor() const { return m_samplerDescriptor; }

protected:
	Descriptor m_samplerDescriptor;
};

} // namespace Luna::DX12