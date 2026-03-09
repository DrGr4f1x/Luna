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
    float4 color : COLOR;
};


struct VSOutput
{
    float4 pos : SV_Position;
    float3 normal : NORMAL;
    float3 color : COLOR;
    float3 viewVec : TEXCOORD0;
    float3 lightVec : TEXCOORD1;
    float3 normalVS : TEXCOORD2;
    float3 normalSS : TEXCOORD3;
};


cbuffer VSConstants : BINDING(b0, 0)
{
    float4x4 projectionMatrix;
    float4x4 modelMatrix;
    float4x4 modelViewMatrix;
    float4 lightPos;
    float3 modelColor;
};


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    output.normal = mul((float3x3) modelMatrix, input.normal);
    output.normalVS = mul((float3x3) modelViewMatrix, input.normal);
    output.normalSS = mul((float3x3) projectionMatrix, input.normal);
    output.color = modelColor;

    float4 pos = mul(modelMatrix, float4(input.pos, 1.0f));

    output.lightVec = lightPos.xyz - pos.xyz;
    output.viewVec = -pos.xyz;

    output.pos = mul(projectionMatrix, pos);

    return output;
}