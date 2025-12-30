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
    float4 color    : COLOR;
    float2 uv       : TEXCOORD;
};


cbuffer ubo : BINDING(b0, 0)
{
    float4x4 projection;
    float4x4 view;
    float4x4 model;
    float4x4 lightSpace;
    float4 lightPos;
    float zNear;
    float zFar;
}


struct VSOutput
{
    float4 pos          : SV_POSITION;
    float3 normal       : NORMAL;
    float3 color        : COLOR;
    float3 viewVec      : TEXCOORD0;
    float3 lightVec     : TEXCOORD1;
    float4 shadowCoord  : TEXCOORD2;
};


static const float4x4 biasMat = float4x4(
	0.5, 0.0, 0.0, 0.5,
	0.0, -0.5, 0.0, 0.5,
	0.0, 0.0, 1.0, 0.0,
	0.0, 0.0, 0.0, 1.0);


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;
    output.color = input.color.xyz;

    output.pos = mul(projection, mul(view, mul(model, float4(input.pos.xyz, 1.0))));

    float4 pos = mul(model, float4(input.pos, 1.0));
    output.normal = mul((float3x3)model, input.normal);
    output.lightVec = normalize(lightPos.xyz - input.pos);
    output.viewVec = -pos.xyz;

    output.shadowCoord = mul(biasMat, mul(lightSpace, mul(model, float4(input.pos, 1.0))));
    return output;
}