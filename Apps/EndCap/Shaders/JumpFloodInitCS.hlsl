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

Texture2D contourTex : BINDING(t0, 0);
Texture2D normalTex : BINDING(t1, 0);
RWTexture2D<float4> outDataTex : BINDING(u0, 0);
RWTexture2D<uint> outClassTex : BINDING(u1, 0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadId)
{
    float r = contourTex[DTid.xy].x;
    
    float4 outData = float4(-1.0, -1.0, 0.0, 0.0);
    uint outClass = (r > 0.0) ? 1 : 0;
    
    if (outClass == 1)
    {
        outData.x = (float) DTid.x;
        outData.y = (float) DTid.y;
        
        outData.zw = normalTex[DTid.xy].xy;
    }
    
    outDataTex[DTid.xy] = outData;
    outClassTex[DTid.xy] = outClass;
}