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


cbuffer VSConstants : BINDING(b0, 0)
{
    float4x4 projectionMatrix;
    float4x4 viewMatrix;
};


cbuffer VSModelConstants : BINDING(b1, 0)
{
    float4x4 modelMatrix;
};


struct VSInput
{
    float3 pos : POSITION;
    float3 color : COLOR;
};


struct VSOutput
{
    float4 pos : SV_Position;
    float3 color : COLOR;
};


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    output.color = input.color;
    output.pos = mul(projectionMatrix, mul(viewMatrix, mul(modelMatrix, float4(input.pos, 1.0))));

    return output;
}
