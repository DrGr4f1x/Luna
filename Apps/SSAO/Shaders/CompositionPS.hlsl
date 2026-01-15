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


Texture2D textureposition : BINDING(t0, 0);
Texture2D textureNormal : BINDING(t1, 0);
Texture2D textureAlbedo : BINDING(t2, 0);
Texture2D textureSSAO : BINDING(t3, 0);
Texture2D textureSSAOBlur : BINDING(t4, 0);

SamplerState samplerCommon : BINDING(s0, 1);


struct UBO
{
    float4x4 _dummy;
    int ssao;
    int ssaoOnly;
    int ssaoBlur;
};


ConstantBuffer<UBO> uboParams : BINDING(b0, 0);


float4 main(PSInput input) : SV_TARGET
{
    float3 fragPos = textureposition.Sample(samplerCommon, input.uv).rgb;
    float3 normal = normalize(textureNormal.Sample(samplerCommon, input.uv).rgb * 2.0 - 1.0);
    float4 albedo = textureAlbedo.Sample(samplerCommon, input.uv);

    float ssao = (uboParams.ssaoBlur == 1) ? textureSSAOBlur.Sample(samplerCommon, input.uv).r : textureSSAO.Sample(samplerCommon, input.uv).r;

    float3 lightPos = float3(0.0, 0.0, 0.0);
    float3 L = normalize(lightPos - fragPos);
    float NdotL = max(0.5, dot(normal, L));

    float4 outFragColor;
    if (uboParams.ssaoOnly == 1)
    {
        outFragColor.rgb = ssao.rrr;
    }
    else
    {
        float3 baseColor = albedo.rgb * NdotL;

        if (uboParams.ssao == 1)
        {
            outFragColor.rgb = ssao.rrr;

            if (uboParams.ssaoOnly != 1)
                outFragColor.rgb *= baseColor;
        }
        else
        {
            outFragColor.rgb = baseColor;
        }
    }
    return outFragColor;
}