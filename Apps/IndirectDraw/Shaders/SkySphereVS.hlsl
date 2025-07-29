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
    float3 pos  : POSITION;
    float2 uv   : TEXCOORD;
};


[[vk::binding(0, 0)]]
cbuffer ubo : register(b0)
{
    float4x4 projectionMatrix;
    float4x4 modelViewMatrix;
}


struct VSOutput
{
    float4 pos  : SV_POSITION;
    float2 uv   : TEXCOORD;
};


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;
    output.uv = input.uv;
	// Skysphere always at center, only use rotation part of modelview matrix
    output.pos = mul(projectionMatrix, float4(mul((float3x3)modelViewMatrix, input.pos.xyz), 1));
    return output;
}