//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

cbuffer VSConstants : register(b0)
{
    float4x4 viewProjectionMatrix;
    float4x4 modelMatrix;
};

struct VSInput
{
    float3 pos : POSITION;
    float3 color : COLOR;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    output.color = input.color;
    float4x4 modelToProjection = mul(viewProjectionMatrix, modelMatrix);
    output.position = mul(modelToProjection, float4(input.pos, 1.0));

    return output;
}