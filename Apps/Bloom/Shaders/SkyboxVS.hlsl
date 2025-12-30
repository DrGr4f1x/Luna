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
};


struct VSOutput
{
    float4 pos : SV_Position;
    float3 uvw : TEXCOORD;
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

    output.uvw = input.pos;
    float4x4 modelToProjection = mul(projectionMatrix, mul(viewMatrix, modelMatrix));
    output.pos = mul(modelToProjection, float4(input.pos, 1.0));

    return output;
}