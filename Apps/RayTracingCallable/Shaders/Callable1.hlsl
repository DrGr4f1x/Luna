//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#include "RayTracingCommon.hlsli"


[shader("callable")]
void callable1_main(inout CallData data)
{
    // Generate a checker board pattern
    float2 pos = float2(DispatchRaysIndex().x / 8, DispatchRaysIndex().y / 8);
    float col = (pos.x + (pos.y % 2.0)) % 2.0;
    data.color = float3(col, col, col);
}