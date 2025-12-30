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
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};


struct VSOutput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 viewVec : TEXCOORD1;
    float3 lightVec : TEXCOORD2;
};


cbuffer VSConstants : BINDING(b0, 0)
{
    float4x4 projectionMatrix;
    float4x4 modelViewMatrix;
    float4 lightPos;
};


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    float4 pos = float4(input.pos, 1.0);
    float4 eyePos = mul(modelViewMatrix, pos);

    output.pos = mul(projectionMatrix, eyePos);
    output.uv = input.uv;
    output.normal = input.normal;
    output.lightVec = lightPos.xyz - pos.xyz;
    output.viewVec = -pos.xyz;

    return output;
}