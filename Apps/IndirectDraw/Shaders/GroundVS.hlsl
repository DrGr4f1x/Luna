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


struct VSInput
{
    float3 pos      : POSITION;
    float3 normal   : NORMAL;
    float4 color    : COLOR;
    float2 uv       : TEXCOORD;
};


cbuffer ubo : register(b0 VK_DESCRIPTOR_SET(0))
{
    float4x4 projectionMatrix;
    float4x4 modelViewMatrix;
}


struct VSOutput
{
    float4 pos  : SV_POSITION;
    float2 uv   : TEXCOORD;
};


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;
    output.uv = input.uv * 32.0;
    output.pos = mul(projectionMatrix, mul(modelViewMatrix, float4(input.pos.xyz, 1.0)));
    return output;
}