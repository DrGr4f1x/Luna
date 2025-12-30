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
    float2 pos : POSITION;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};


struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
    float4 color : COLOR;
};


cbuffer VSConstants : register(b0 VK_DESCRIPTOR_SET(0))
{
    float4x4 projectionMatrix;
}


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    output.uv = input.uv;
    output.color = input.color;
    output.pos = mul(projectionMatrix, float4(input.pos.xy, 0.0, 1.0));

    return output;
}