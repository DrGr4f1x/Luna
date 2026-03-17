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

Texture2D contourDataTex : BINDING(t0, 0);
RWTexture2D<float4> outDataTex : BINDING(u0, 0);
RWTexture2D<uint> outClassTex : BINDING(u1, 0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadId)
{
    float4 contourData = contourDataTex[DTid.xy];
    
    float4 outData = float4(-1.0, -1.0, 0.0, 0.0);
    //uint outClass = (contourData.z > 0.0) ? 1 : 0;
    uint outClass = (uint) contourData.w;
    
    if (outClass > 0)
    {
        outData.x = (float) DTid.x;
        outData.y = (float) DTid.y;
        
        outData.zw = contourData.xy;
    }
    
    outDataTex[DTid.xy] = outData;
    outClassTex[DTid.xy] = outClass;
}