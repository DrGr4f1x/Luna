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

Texture2D<uint> inputClassTex : BINDING(t0, 0);
Texture2D<float4> contourTex : BINDING(t1, 0);
RWTexture2D<uint> outputClassTex : BINDING(u0, 0);
SamplerState clampSampler : register(s0);

struct Constants
{
    float2 texDimensions;
    float2 invTexDimensions;
};

ConstantBuffer<Constants> Globals : BINDING(b0, 0);

groupshared uint gs_C[100];


uint Max3(uint a, uint b, uint c)
{
    return max(max(a, b), c);
}


uint Min3(uint a, uint b, uint c)
{
    return min(min(a, b), c);
}


uint Med3(uint a, uint b, uint c)
{
    return clamp(a, min(b, c), max(b, c));
}


uint Med9(uint x0, uint x1, uint x2,
            uint x3, uint x4, uint x5,
            uint x6, uint x7, uint x8)
{
    uint A = Max3(Min3(x0, x1, x2), Min3(x3, x4, x5), Min3(x6, x7, x8));
    uint B = Min3(Max3(x0, x1, x2), Max3(x3, x4, x5), Max3(x6, x7, x8));
    uint C = Med3(Med3(x0, x1, x2), Med3(x3, x4, x5), Med3(x6, x7, x8));
    return Med3(A, B, C);
}

[numthreads(8, 8, 1)]
void main(uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID)
{
    uint2 st = DTid.xy;
    
    if (GTid.x < 5 && GTid.y < 5)
    {
        float2 prefetchUV = (st + GTid.xy) * Globals.invTexDimensions;
        uint4 C = inputClassTex.GatherRed(clampSampler, prefetchUV);
        
        uint destIdx = GTid * 2 + GTid.y * 2 * 10;
        
        gs_C[destIdx] = (C.w == 1) ? 1 : 0;
        gs_C[destIdx + 1] = (C.z == 1) ? 1 : 0;
        gs_C[destIdx + 10] = (C.x == 1) ? 1 : 0;
        gs_C[destIdx + 11] = (C.y == 1) ? 1 : 0;

    }
    
    GroupMemoryBarrierWithGroupSync();
    
    uint ulIdx = GTid.x + GTid.y * 10;

    uint medC = Med9(
        gs_C[ulIdx], gs_C[ulIdx + 1], gs_C[ulIdx + 2],
        gs_C[ulIdx + 10], gs_C[ulIdx + 11], gs_C[ulIdx + 12],
        gs_C[ulIdx + 20], gs_C[ulIdx + 21], gs_C[ulIdx + 22]);
    
    uint originalClass = contourTex[DTid.xy].r > 0.0 ? 1 : 0;
    
    outputClassTex[st] = originalClass == 1 ? 1 : medC;
}