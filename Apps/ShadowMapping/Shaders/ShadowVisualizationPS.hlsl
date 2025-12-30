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

Texture2D textureColor : register(t0 VK_DESCRIPTOR_SET(0));
SamplerState samplerColor : register(s0 VK_DESCRIPTOR_SET(1));


cbuffer ubo : register(b0 VK_DESCRIPTOR_SET(0))
{
    float4x4 projection;
    float4x4 view;
    float4x4 model;
    float4x4 lightSpace;
    float4 lightPos;
    float zNear;
    float zFar;
}


struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};


float LinearizeDepth(float depth)
{
    float n = zNear;
    float f = zFar;
    float z = depth;
    return (2.0 * n) / (f + n - z * (f - n));
}


float4 main(PSInput input) : SV_TARGET
{
    float depth = textureColor.Sample(samplerColor, input.uv).r;
    return float4((1.0 - LinearizeDepth(depth)).xxx, 1.0);
}