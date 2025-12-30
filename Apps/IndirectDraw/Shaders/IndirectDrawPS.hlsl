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


Texture2DArray textureArray : register(t0 VK_DESCRIPTOR_SET(1));
SamplerState samplerArray : register(s0 VK_DESCRIPTOR_SET(2));


struct PSInput
{
    float4 pos          : SV_POSITION;
    float3 normal       : NORMAL;
    float4 color        : COLOR;
    float3 uv           : TEXCOORD0;
    float3 viewVec      : TEXCOORD1;
    float3 lightVec     : TEXCOORD2;
};


float4 main(PSInput input) : SV_TARGET
{
    float4 color = textureArray.Sample(samplerArray, input.uv);

    if (color.a < 0.5)
    {
        clip(-1);
    }

    float3 N = normalize(input.normal);
    float3 L = normalize(input.lightVec);
    float3 ambient = float3(0.65, 0.65, 0.65);
    float3 diffuse = max(dot(N, L), 0.0) * input.color.rgb;
    return float4((ambient + diffuse) * color.rgb, 1.0);
}