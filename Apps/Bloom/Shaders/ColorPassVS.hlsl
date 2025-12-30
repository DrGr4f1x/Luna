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
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};


struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
    float3 color : COLOR;
};


cbuffer VSConstants : BINDING(b0, 0)
{
    float4x4 projectionMatrix;
    float4x4 viewMatrix;
    float4x4 modelMatrix;
};


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    output.color = input.color.xyz;
    output.uv = input.uv;

    float4x4 modelToProjection = mul(projectionMatrix, mul(viewMatrix, modelMatrix));
    output.pos = mul(modelToProjection, float4(input.pos.xyz, 1.0f));

    return output;
}