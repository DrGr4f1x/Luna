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
    float3 tangent  : TEXCOORD0;
    float2 uv       : TEXCOORD1;
};


struct VSOutput
{
    float4 pos              : SV_POSITION;
    float2 uv               : TEXCOORD0;
    float3 tangentLightPos  : TEXCOORD1;
    float3 tangentViewPos   : TEXCOORD2;
    float3 tangentFragPos   : TEXCOORD3;
};


cbuffer VSConstants : BINDING(b0, 0)
{
    float4x4 projection;
    float4x4 view;
    float4x4 model;
    float4 lightPos;
    float4 cameraPos;
};


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;
    output.pos = mul(projection, mul(view, mul(model, float4(input.pos, 1.0f))));
    output.uv = input.uv;

    float3 N = normalize(input.normal);
    float3 T = normalize(input.tangent.xyz);
    float3 B = normalize(cross(N, T));
    float3x3 TBN = float3x3(T, B, N);

    output.tangentLightPos = mul(lightPos.xyz, TBN);
    output.tangentViewPos = mul(cameraPos.xyz, TBN);
    output.tangentFragPos = mul(mul(model, float4(input.pos, 1.0)).xyz, TBN);
    return output;
}