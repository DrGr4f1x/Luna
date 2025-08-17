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
    float3 pos          : POSITION;
    float2 uv           : TEXCOORD;
    int textureIndex    : TEXTUREINDEX;
};


[[vk::binding(0, 0)]]
cbuffer matrices : register(b0)
{
    float4x4 projection;
    float4x4 view;
    float4x4 model;
};


struct VSOutput
{
    float4 pos          : SV_POSITION;
    float2 uv           : TEXCOORD;
    int textureIndex    : TEXTUREINDEX;
};


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;
    output.uv = input.uv;
    output.textureIndex = input.textureIndex;
    output.pos = mul(projection, mul(view, mul(model, float4(input.pos.xyz, 1.0))));
    return output;
}