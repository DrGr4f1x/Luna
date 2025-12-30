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
    uint instanceId : SV_InstanceId;
};


struct VSOutput
{
    float4 pos : SV_Position;
    float3 uv : TEXCOORD;
};


struct InstanceData
{
    float4x4 modelMatrix;
    float4 arrayIndex;
};


cbuffer VSConstants : BINDING(b0, 0)
{
    float4x4 viewProjectionMatrix;
    InstanceData instance[8];
}


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    output.uv = float3(input.uv, instance[input.instanceId].arrayIndex.x);

    float4x4 modelToProjection = mul(viewProjectionMatrix, instance[input.instanceId].modelMatrix);
    output.pos = mul(modelToProjection, float4(input.pos, 1.0));

    return output;
}