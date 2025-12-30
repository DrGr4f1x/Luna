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

struct GSInput
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
};


struct GSOutput
{
    float4 pos : SV_Position;
    float3 color : COLOR;
};


cbuffer GSConstants : register(b0 VK_DESCRIPTOR_SET(0))
{
    float4x4 projectionMatrix;
    float4x4 modelMatrix;
};


[maxvertexcount(6)]
void main(triangle GSInput input[3], inout LineStream<GSOutput> outputStream)
{
    GSOutput output = (GSOutput) 0;

    float normalLength = 0.1f;

    for (int i = 0; i < 3; ++i)
    {
        float3 pos = input[i].pos;
        float3 normal = input[i].normal;

        output.pos = mul(projectionMatrix, mul(modelMatrix, float4(pos, 1.0f)));
        output.color = float3(1.0f, 0.0f, 0.0f);
        outputStream.Append(output);

        output.pos = mul(projectionMatrix, mul(modelMatrix, float4(pos + normal * normalLength, 1.0f)));
        output.color = float3(0.0f, 0.0f, 1.0f);
        outputStream.Append(output);

        outputStream.RestartStrip();
    }
}