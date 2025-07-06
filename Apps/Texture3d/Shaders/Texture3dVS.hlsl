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
    float2 uv : TEXCOORD;
};


struct VSOutput
{
    float4 pos : SV_Position;
    float3 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 viewVec : TEXCOORD1;
    float3 lightVec : TEXCOORD2;
};


VK_BINDING(0, 0)
cbuffer VSConstants : register(b0)
{
    float4x4 viewProjectionMatrix;
    float4x4 modelMatrix;
    float4 viewPos;
    float depth;
};


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    float3 worldPos = mul(modelMatrix, float4(input.pos, 1.0f)).xyz;
    output.pos = mul(viewProjectionMatrix, mul(modelMatrix, float4(input.pos, 1.0f)));
    output.normal = mul((float3x3) modelMatrix, input.normal);
    output.uv = float3(input.uv, depth);

    float3 lightPos = 0.0.xxx;
    float3 lPos = mul((float3x3) modelMatrix, lightPos);
    output.lightVec = lPos - worldPos;
    output.viewVec = viewPos.xyz - worldPos;

    return output;
}