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
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(2)]] float3 color : COLOR;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD;
    float3 color : COLOR;
};

[[vk::binding(0)]]
cbuffer VSConstants : register(b0)
{
    float4x4 projectionMatrix;
    float4x4 modelMatrix;
    float gradientPos;
};

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    output.color = input.color;
    output.texcoord = float2(gradientPos, 0.0f);

    float4x4 modelToProjection = mul(projectionMatrix, modelMatrix);
    output.position = mul(modelToProjection, float4(input.position, 1.0f));

    return output;
}