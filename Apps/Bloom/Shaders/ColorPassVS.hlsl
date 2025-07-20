//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

struct VSInput
{
    float4 pos : POSITION;
    float2 uv : TEXCOORD;
    float3 color : COLOR;
};


struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD;
    float3 color : COLOR;
};


[[vk::binding(0,0)]]
cbuffer VSConstants : register(b0)
{
    float4x4 projectionMatrix;
    float4x4 viewMatrix;
    float4x4 modelMatrix;
};


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    output.color = input.color;
    output.uv = input.uv;

    float4x4 modelToProjection = mul(projectionMatrix, mul(viewMatrix, modelMatrix));
    output.pos = mul(modelToProjection, float4(input.pos.xyz, 1.0f));

    return output;
}