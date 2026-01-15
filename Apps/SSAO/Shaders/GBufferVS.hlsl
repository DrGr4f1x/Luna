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
    float3 pos      : POSITION0;
    float3 normal   : NORMAL0;
    float3 color    : COLOR0;
    float2 uv       : TEXCOORD0;
};


struct VSOutput
{
    float4 pos          : SV_POSITION;
    float3 normal       : NORMAL0;
    float2 uv           : TEXCOORD0;
    float3 color        : COLOR0;
    float3 worldPos     : POSITION0;
};


struct UBO
{
    float4x4 projection;
    float4x4 model;
    float4x4 view;
    float nearPlane;
    float farPlane;
};


ConstantBuffer<UBO> ubo : BINDING(b0, 0);


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;
    
    output.pos = mul(ubo.projection, mul(ubo.view, mul(ubo.model, float4(input.pos, 1.0))));

    output.uv = input.uv;

	// Vertex position in view space
    output.worldPos = mul(ubo.view, mul(ubo.model, float4(input.pos, 1.0))).xyz;

	// Normal in view space
    float3x3 normalMatrix = (float3x3) mul(ubo.view, ubo.model);
    output.normal = mul(normalMatrix, input.normal);

    output.color = input.color;
    
    return output;
}