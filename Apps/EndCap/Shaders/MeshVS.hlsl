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
    float4 color : COLOR;
    float clipDistance : SV_ClipDistance;
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
    float4 clipPlane;
    float alpha;
};


struct Model
{
    float3 posOffset;
    int applyCut;
};
[[vk::push_constant]]
ConstantBuffer<Model> Model : register(b1);


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    output.normal = mul((float3x3) modelMatrix, input.normal);
    output.normalVS = mul((float3x3) modelViewMatrix, input.normal);
    output.normalSS = mul((float3x3) projectionMatrix, input.normal);
    output.color = float4(modelColor, alpha);

    float4 pos = mul(modelMatrix, float4(input.pos + Model.posOffset, 1.0f));

    output.lightVec = lightPos.xyz - pos.xyz;
    output.viewVec = -pos.xyz;

    output.pos = mul(projectionMatrix, pos);

    output.clipDistance = 1.0;
    if (Model.applyCut > 0)
    {
        // We're keeping points below the plane, so take the negative
        output.clipDistance = -(dot(pos.xyz, clipPlane.xyz) + clipPlane.w);

    }
    
    return output;
}