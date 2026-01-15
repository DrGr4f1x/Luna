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


struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
};


Texture2D textureSSAO : BINDING(t0, 0);
SamplerState samplerSSAO : BINDING(s0, 1);


float4 main(PSInput input) : SV_TARGET
{
    const int blurRange = 2;
    int n = 0;
    int2 texDim;
    textureSSAO.GetDimensions(texDim.x, texDim.y);
    float2 texelSize = 1.0 / (float2) texDim;
    float result = 0.0;
    for (int x = -blurRange; x <= blurRange; x++)
    {
        for (int y = -blurRange; y <= blurRange; y++)
        {
            float2 offset = float2(float(x), float(y)) * texelSize;
            result += textureSSAO.Sample(samplerSSAO, input.uv + offset).r;
            n++;
        }
    }
    return result / (float(n));
}