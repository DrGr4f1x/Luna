//
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Author:  David Elder
//

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
    float3 lightVec : TEXCOORD;
};


cbuffer VSConstants : register(b0)
{
    float4x4 viewProjectionMatrix;
    float4x4 modelMatrix;
    float4 lightPos;
};


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    output.color = float3(1.0f, 0.0f, 0.0f);

    output.pos = mul(viewProjectionMatrix, mul(modelMatrix, float4(input.pos, 1.0f)));
    output.normal = mul((float3x3) modelMatrix, input.normal);

    float4 pos = mul(modelMatrix, float4(input.pos, 1.0f));
    float3 lPos = mul(modelMatrix, lightPos).xyz;
    output.lightVec = lPos - pos.xyz;

    return output;
}