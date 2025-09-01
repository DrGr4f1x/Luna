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

#include "Graphics\DeviceCaps.h"
#include "Graphics\DX12\DirectXCommon.h"


namespace Luna::DX12
{

void FillCaps(ID3D12Device* device, DeviceCaps& caps);

} // namespace Luna::DX12