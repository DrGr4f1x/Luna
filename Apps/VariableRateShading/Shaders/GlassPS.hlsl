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
    float4 position             : SV_POSITION;
    float4 worldpos             : POSITION;
    float2 uv                   : TEXCOORD0;
    float3 normal               : NORMAL;
    float3 tangent              : TANGENT;
    float4 refractionPosition   : TEXCOORD1;
};


Texture2D diffuseMap : BINDING(t0, 1);
Texture2D normalMap : BINDING(t1, 1);
Texture2D shadowMap : BINDING(t2, 1);
Texture2D refractionMap : BINDING(t3, 1);

SamplerState sampleClamp : BINDING(s0, 2);
SamplerState sampleWrap : BINDING(s1, 2);


cbuffer GlassConstantBuffer : BINDING(b1, 1)
{
    float refractionScale;
    bool flipNormal;
}


#include "Lighting.hlsli"


float4 main(PSInput input) : SV_TARGET
{
    float4 diffuseColor = diffuseMap.Sample(sampleWrap, input.uv);
    float3 pixelNormal = CalcPerPixelNormal(input.uv, input.normal, input.tangent);
    float4 totalLight = ambientColor;

    if (flipNormal)
    {
        pixelNormal.z = -pixelNormal.z;
    }

    for (int i = 0; i < NUM_LIGHTS; i++)
    {
        float4 lightPass = CalcLightingColor(lights[i].position, lights[i].direction, lights[i].color, lights[i].falloff, input.worldpos.xyz, pixelNormal);
        if (sampleShadowMap && i == 0)
        {
            lightPass *= CalcUnshadowedAmountPCF2x2(i, input.worldpos);
        }
        totalLight += lightPass;
    }

    // Bump up the blue color a bit.
    diffuseColor.z += .3f;

    float2 refractionUV;
    refractionUV.x = input.refractionPosition.x / input.refractionPosition.w / 2.0f + 0.5f;
    refractionUV.y = -input.refractionPosition.y / input.refractionPosition.w / 2.0f + 0.5f;
    refractionUV = refractionUV + (pixelNormal.xy * refractionScale);

    // Blend refraction color and diffuse color.
    float4 refractionColor = refractionMap.Sample(sampleClamp, refractionUV);
    diffuseColor = lerp(refractionColor, diffuseColor * refractionColor, .5f);

    return diffuseColor * saturate(totalLight);
}