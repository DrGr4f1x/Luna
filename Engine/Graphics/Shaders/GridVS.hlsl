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
    float4 color : COLOR;
};


struct VSOutput
{
    float4 pos : SV_Position;
    float3 color : COLOR;
};


cbuffer VSConstants : register(b0)
{
    float4x4 viewProjectionMatrix;
};


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    output.pos = mul(viewProjectionMatrix, float4(input.pos, 1.0));
    output.color = input.color.xyz;

    return output;
}