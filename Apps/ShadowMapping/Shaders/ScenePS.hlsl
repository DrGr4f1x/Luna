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

Texture2D shadowMapTexture : register(t0 VK_DESCRIPTOR_SET(1));
SamplerState shadowMapSampler : register(s0 VK_DESCRIPTOR_SET(2));


struct PSInput
{
    float4 pos          : SV_POSITION;
    float3 normal       : NORMAL;
    float3 color        : COLOR;
    float3 viewVec      : TEXCOORD0;
    float3 lightVec     : TEXCOORD1;
    float4 shadowCoord  : TEXCOORD2;
};

#define ambient 0.1


float textureProj(float4 shadowCoord, float2 off)
{
    float shadow = 1.0;
    if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0)
    {
        float dist = shadowMapTexture.Sample(shadowMapSampler, shadowCoord.xy + off).r;
        if (shadowCoord.w > 0.0 && dist < shadowCoord.z)
        {
            shadow = ambient;
        }
    }
    return shadow;
}


float filterPCF(float4 sc)
{
    int2 texDim;
    shadowMapTexture.GetDimensions(texDim.x, texDim.y);
    float scale = 1.5;
    float dx = scale * 1.0 / float(texDim.x);
    float dy = scale * 1.0 / float(texDim.y);

    float shadowFactor = 0.0;
    int count = 0;
    int range = 1;

    for (int x = -range; x <= range; x++)
    {
        for (int y = -range; y <= range; y++)
        {
            shadowFactor += textureProj(sc, float2(dx * x, dy * y));
            count++;
        }

    }
    return shadowFactor / count;
}


float4 main(PSInput input) : SV_TARGET
{
    const int enablePCF = 1;
    float shadow = (enablePCF == 1) ? filterPCF(input.shadowCoord / input.shadowCoord.w) : textureProj(input.shadowCoord / input.shadowCoord.w, float2(0.0, 0.0));

    float3 N = normalize(input.normal);
    float3 L = normalize(input.lightVec);
    float3 V = normalize(input.viewVec);
    float3 R = normalize(-reflect(L, N));
    float3 diffuse = max(dot(N, L), ambient) * input.color;

    return float4(diffuse * shadow, 1.0);
}