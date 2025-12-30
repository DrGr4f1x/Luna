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
    float3 pos              : POSITION;
    float3 normal           : NORMAL;
    float4 color            : COLOR;
    float2 uv               : TEXCOORD0;
    float3 instancePos      : INSTANCED_POSITION;
    float3 instanceRot      : INSTANCED_ROTATION;
    float instanceScale     : INSTANCED_SCALE;
    int instanceTexIndex   : INSTANCED_TEX_INDEX;
};


cbuffer ubo : BINDING(b0, 0)
{
    float4x4 projectionMatrix;
    float4x4 modelViewMatrix;
}


struct VSOutput
{
    float4 pos          : SV_POSITION;
    float3 normal       : NORMAL;
    float4 color        : COLOR;
    float3 uv           : TEXCOORD0;
    float3 viewVec      : TEXCOORD1;
    float3 lightVec     : TEXCOORD2;
};


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;
    output.color = input.color;
    output.uv = float3(input.uv, input.instanceTexIndex);

    float4x4 mx, my, mz;

	// rotate around x
    float s = sin(input.instanceRot.x);
    float c = cos(input.instanceRot.x);

    mx[0] = float4(c, s, 0.0, 0.0);
    mx[1] = float4(-s, c, 0.0, 0.0);
    mx[2] = float4(0.0, 0.0, 1.0, 0.0);
    mx[3] = float4(0.0, 0.0, 0.0, 1.0);

	// rotate around y
    s = sin(input.instanceRot.y);
    c = cos(input.instanceRot.y);

    my[0] = float4(c, 0.0, s, 0.0);
    my[1] = float4(0.0, 1.0, 0.0, 0.0);
    my[2] = float4(-s, 0.0, c, 0.0);
    my[3] = float4(0.0, 0.0, 0.0, 1.0);

	// rot around z
    s = sin(input.instanceRot.z);
    c = cos(input.instanceRot.z);

    mz[0] = float4(1.0, 0.0, 0.0, 0.0);
    mz[1] = float4(0.0, c, s, 0.0);
    mz[2] = float4(0.0, -s, c, 0.0);
    mz[3] = float4(0.0, 0.0, 0.0, 1.0);

    float4x4 rotMat = mul(mz, mul(my, mx));

    output.normal = mul((float4x3) rotMat, input.normal).xyz;

    float4 pos = mul(rotMat, float4((input.pos.xyz * input.instanceScale) + input.instancePos.xyz, 1.0));

    output.pos = mul(projectionMatrix, mul(modelViewMatrix, pos));

    float4 wPos = mul(modelViewMatrix, float4(pos.xyz, 1.0));
    float4 lPos = float4(0.0, 5.0, 0.0, 1.0);
    output.lightVec = lPos.xyz - pos.xyz;
    output.viewVec = -pos.xyz;
    return output;
}