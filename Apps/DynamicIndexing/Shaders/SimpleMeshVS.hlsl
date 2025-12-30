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
    float3 position     : POSITION;
    float3 normal       : NORMAL;
    float2 uv           : TEXCOORD0;
    float3 tangent      : TANGENT;
};


struct VSOutput
{
    float4 position     : SV_POSITION;
    float2 uv           : TEXCOORD0;
};


cbuffer cb0 : register(b0 VK_DESCRIPTOR_SET(0))
{
    float4x4 modelViewProjectionMatrix;
};


VSOutput main(VSInput input)
{
    VSOutput result = (VSOutput) 0;
    
    result.position = mul(modelViewProjectionMatrix, float4(input.position, 1.0f));
    result.uv = input.uv;
    
    return result;
}