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
};


cbuffer VSConstants : BINDING(b0, 0)
{
    float4x4 viewProjectionMatrix;
    float4x4 modelMatrix;
    float4 lightPos;
    float outlineWidth;
};


float4 main(VSInput input) : SV_POSITION
{
    float4 pos = float4(input.pos.xyz + input.normal * outlineWidth, 1.0);
    pos = mul(viewProjectionMatrix, mul(modelMatrix, pos));

    return pos;
}