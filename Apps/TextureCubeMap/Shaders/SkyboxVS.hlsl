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

struct VSOutput
{
    float4 pos : SV_Position;
    float3 uvw : TEXCOORD;
};


cbuffer VSConstants : register(b0 VK_DESCRIPTOR_SET(0))
{
    float4x4 viewProjectionMatrix;
    float4x4 modelMatrix;
    float3 eyePos;
}


VSOutput main(float3 pos : POSITION)
{
    VSOutput output = (VSOutput) 0;

    output.pos = mul(viewProjectionMatrix, mul(modelMatrix, float4(pos, 1.0f)));

    output.uvw = pos.xyz;
    output.uvw.xy *= -1.0f;

    return output;
}