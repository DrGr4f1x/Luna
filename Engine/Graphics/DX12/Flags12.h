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

#include "Core\FlagStringMap.h"

namespace Luna::DX12
{

inline static const FlagStringMap32 g_commandListSupportFlagsMap{
	{ D3D12_COMMAND_LIST_SUPPORT_FLAG_NONE, "None" },
	{ D3D12_COMMAND_LIST_SUPPORT_FLAG_DIRECT, "Direct" },
	{ D3D12_COMMAND_LIST_SUPPORT_FLAG_BUNDLE, "Bundle" },
	{ D3D12_COMMAND_LIST_SUPPORT_FLAG_COMPUTE, "Compute" },
	{ D3D12_COMMAND_LIST_SUPPORT_FLAG_COPY, "Copy" },
	{ D3D12_COMMAND_LIST_SUPPORT_FLAG_VIDEO_DECODE, "Video Decode" },
	{ D3D12_COMMAND_LIST_SUPPORT_FLAG_VIDEO_PROCESS, "Video Process" },
	{ D3D12_COMMAND_LIST_SUPPORT_FLAG_VIDEO_ENCODE, "Video Encode" }
};

} // namespace Luna::DX12
