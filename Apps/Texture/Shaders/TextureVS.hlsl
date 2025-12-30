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
    float2 uv       : TEXCOORD;
    float3 normal   : NORMAL;
};


struct VSOutput
{
    float4 pos          : SV_Position;
    float2 uv           : TEXCOORD0;
    float lodBias       : TEXCOORD1;
    float3 normal       : NORMAL;
    float3 viewVec      : TEXCOORD2;
    float3 lightVec     : TEXCOORD3;
};


cbuffer VSConstants : register(b0 VK_DESCRIPTOR_SET(0))
{
    float4x4 viewProjectionMatrix;
    float4x4 modelMatrix;
    float4 viewPos;
    float lodBias;
    bool flipUVs;
};


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    output.uv = input.uv;
    if (flipUVs)
        output.uv.y = 1.0 - output.uv.y;
    
    output.lodBias = lodBias;

    float3 worldPos = mul(modelMatrix, float4(input.pos, 1.0f)).xyz;

    float4x4 modelToProjection = mul(viewProjectionMatrix, modelMatrix);
    output.pos = mul(modelToProjection, float4(input.pos, 1.0f));
	
    output.normal = mul(transpose(modelMatrix), float4(input.normal, 0.0f)).xyz;

    float3 lightPos = viewPos.xyz;
    lightPos = mul(modelMatrix, float4(lightPos, 0.0f)).xyz;

    output.lightVec = lightPos - worldPos;
    output.viewVec = viewPos.xyz - worldPos;

    return output;
}