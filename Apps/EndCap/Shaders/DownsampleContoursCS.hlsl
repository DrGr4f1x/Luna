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

Texture2D<uint> edgeIdTex : BINDING(t0, 0);
RWTexture2D<uint> outEdgeTex : BINDING(u0, 0);

struct Constants_t
{
    uint width;
    uint height;
};

[[vk::push_constant]]
ConstantBuffer<Constants_t> Constants : register(b0);

[numthreads(8, 8, 1)]
void main(uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID)
{
    if(DTid.x >= Constants.width || DTid.y >= Constants.height)
        return;
    
    bool laneContainsEdge = edgeIdTex[DTid.xy].r > 0;
    
    bool anyLaneContainsEdge = WaveActiveAnyTrue(laneContainsEdge);
    if (anyLaneContainsEdge && WaveIsFirstLane())
    {
        outEdgeTex[Gid.xy] = 1;
    }
}