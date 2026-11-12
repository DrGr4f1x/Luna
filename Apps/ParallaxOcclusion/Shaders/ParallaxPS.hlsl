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
    float4 pos              : SV_POSITION;
    float2 uv               : TEXCOORD0;
    float3 tangentLightPos  : TEXCOORD1;
    float3 tangentViewPos   : TEXCOORD2;
    float3 tangentFragPos   : TEXCOORD3;
};


cbuffer PSConstants : BINDING(b0, 1)
{
    float heightScale;
    float parallaxBias;
    float numLayers;
    int mappingMode;
};


Texture2D textureColorMap : BINDING(t0, 1);
Texture2D textureNormalHeightMap : BINDING(t1, 1);
SamplerState samplerLinearWrap : BINDING(s0, 2);


float2 ParallaxMapping(float2 uv, float3 viewDir)
{
    float height = 1.0 - textureNormalHeightMap.SampleLevel(samplerLinearWrap, uv, 0.0).a;
    float2 p = viewDir.xy * (height * (heightScale * 0.5) + parallaxBias) / viewDir.z;
    return uv - p;
}


float2 SteepParallaxMapping(float2 uv, float3 viewDir)
{
    float layerDepth = 1.0 / numLayers;
    float currLayerDepth = 0.0;
    float2 deltaUV = viewDir.xy * heightScale / (viewDir.z * numLayers);
    float2 currUV = uv;
    float height = 1.0 - textureNormalHeightMap.SampleLevel(samplerLinearWrap, currUV, 0.0).a;
    for (int i = 0; i < numLayers; i++)
    {
        currLayerDepth += layerDepth;
        currUV -= deltaUV;
        height = 1.0 - textureNormalHeightMap.SampleLevel(samplerLinearWrap, currUV, 0.0).a;
        if (height < currLayerDepth)
        {
            break;
        }
    }
    return currUV;
}


float2 ParallaxOcclusionMapping(float2 uv, float3 viewDir)
{
    float layerDepth = 1.0 / numLayers;
    float currLayerDepth = 0.0;
    float2 deltaUV = viewDir.xy * heightScale / (viewDir.z * numLayers);
    float2 currUV = uv;
    float height = 1.0 - textureNormalHeightMap.SampleLevel(samplerLinearWrap, currUV, 0.0).a;
    for (int i = 0; i < numLayers; i++)
    {
        currLayerDepth += layerDepth;
        currUV -= deltaUV;
        height = 1.0 - textureNormalHeightMap.SampleLevel(samplerLinearWrap, currUV, 0.0).a;
        if (height < currLayerDepth)
        {
            break;
        }
    }
    float2 prevUV = currUV + deltaUV;
    float nextDepth = height - currLayerDepth;
    float prevDepth = 1.0 - textureNormalHeightMap.SampleLevel(samplerLinearWrap, prevUV, 0.0).a - currLayerDepth + layerDepth;
    return lerp(currUV, prevUV, nextDepth / (nextDepth - prevDepth));
}


float4 main(PSInput input) : SV_TARGET
{
    float3 V = normalize(input.tangentViewPos - input.tangentFragPos);
    float2 uv = input.uv;

    if (mappingMode == 0)
    {
		// Color only
        return textureColorMap.Sample(samplerLinearWrap, input.uv);
    }
    else
    {
        switch (mappingMode)
        {
            case 2:
                uv = ParallaxMapping(input.uv, V);
                break;
            case 3:
                uv = SteepParallaxMapping(input.uv, V);
                break;
            case 4:
                uv = ParallaxOcclusionMapping(input.uv, V);
                break;
        }

		// Discard fragments at texture border
        if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
        {
            clip(-1);
        }

        float3 N = normalize(textureNormalHeightMap.SampleLevel(samplerLinearWrap, uv, 0.0).rgb * 2.0 - 1.0);
        float3 L = normalize(input.tangentLightPos - input.tangentFragPos);
        float3 R = reflect(-L, N);
        float3 H = normalize(L + V);

        float3 color = textureColorMap.Sample(samplerLinearWrap, uv).rgb;
        float3 ambient = 0.2 * color;
        float3 diffuse = max(dot(L, N), 0.0) * color;
        float3 specular = float3(0.15, 0.15, 0.15) * pow(max(dot(N, H), 0.0), 32.0);

        return float4(ambient + diffuse + specular, 1.0f);
    }
}