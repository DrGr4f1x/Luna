//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

#include "Common.hlsli"

struct GeometryInfo
{
    uint count;
    uint maxNodeCount;
};

RWStructuredBuffer<GeometryInfo> geometryBuffer : BINDING(u0, 0);

[numthreads(1, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadId)
{
    if (DTid.x == 0 && DTid.y == 0)
    {
        geometryBuffer[0].count = 0;
    }
}