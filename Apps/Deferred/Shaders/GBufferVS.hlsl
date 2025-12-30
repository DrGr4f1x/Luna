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
    float3 pos      : POSITION;
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;
    float4 color    : COLOR;
    float2 uv       : TEXCOORD;
};


cbuffer ubo : register(b0 VK_DESCRIPTOR_SET(0))
{
    float4x4 projection;
    float4x4 model;
    float4x4 view;
    float3 instancePos[3];
}


struct VSOutput
{
    float4 pos          : SV_POSITION;
    float3 worldPos     : POSITION;
    float2 uv           : TEXCOORD;
    float4 color        : COLOR;
    float3 normal       : NORMAL;
    float3 tangent      : TANGENT;
};

VSOutput main(VSInput input, uint instanceIndex : SV_InstanceID)
{
    VSOutput output = (VSOutput) 0;
    
    float3 tmpPos = input.pos + instancePos[instanceIndex];
    output.pos = mul(projection, mul(view, mul(model, float4(tmpPos, 1.0))));
    output.uv = input.uv;

	// Vertex position in world space
    output.worldPos = mul(model, tmpPos).xyz;

	// Normal in world space
    output.normal = normalize(input.normal);
    output.tangent = normalize(input.tangent);

	// Currently just vertex color
    output.color = input.color;
    return output;
}
