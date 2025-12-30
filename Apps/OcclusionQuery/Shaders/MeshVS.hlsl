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


cbuffer VSConstants : BINDING(b0, 0)
{
    float4x4 projectionMatrix;
    float4x4 modelViewMatrix;
    float4 lightPos;
    float4 color;
    float visible;
}


struct VSInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
};


struct VSOutput
{
    float4 pos : SV_POSITION;
    float3 normal : NORMAL;
    float3 color : COLOR;
    float3 viewVec : TEXCOORD0;
    float3 lightVec : TEXCOORD1;
    float visible : TEXCOORD2;
};


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    float4 pos = mul(modelViewMatrix, float4(input.pos, 1.0));

    output.pos = mul(projectionMatrix, pos);
    output.normal = mul((float3x3) modelViewMatrix, input.normal);

    output.lightVec = lightPos.xyz - pos.xyz;
    output.viewVec = -pos.xyz;

    output.color = color;
    output.visible = visible;

    return output;
}