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


struct PSInput
{
    float4 pos          : SV_POSITION;
    float3 normal       : NORMAL0;
    float2 uv           : TEXCOORD0;
    float3 color        : COLOR0;
    float3 worldPos     : POSITION0;
};


struct PSOutput
{
    float4 position : SV_TARGET0;
    float4 normal   : SV_TARGET1;
    float4 albedo   : SV_TARGET2;
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


Texture2D textureColorMap : BINDING(t0, 1);
SamplerState samplerColorMap : BINDING(s0, 2);


float linearDepth(float depth)
{
    float z = depth * 2.0f - 1.0f;
    return (2.0f * ubo.nearPlane * ubo.farPlane) / (ubo.farPlane + ubo.nearPlane - z * (ubo.farPlane - ubo.nearPlane));
}


PSOutput main(PSInput input)
{
    PSOutput output = (PSOutput) 0;
    
    output.position = float4(input.worldPos, linearDepth(input.pos.z));
    output.normal = float4(normalize(input.normal) * 0.5 + 0.5, 1.0);
    output.albedo = textureColorMap.Sample(samplerColorMap, input.uv) * float4(input.color, 1.0);
    
    return output;
}