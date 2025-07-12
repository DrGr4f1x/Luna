//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

[[vk::binding(0, 0)]]
cbuffer VSConstants : register(b0)
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
    float3 color : COLOR;
};


struct VSOutput
{
    float4 pos : SV_POSITION;
    float3 color : COLOR;
};


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    output.pos = mul(projectionMatrix, mul(modelViewMatrix, float4(input.pos, 1.0)));
    output.color = color;

    return output;
}