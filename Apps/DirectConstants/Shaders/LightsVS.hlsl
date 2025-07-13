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
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
};

#define LIGHT_COUNT 6

struct VSOutput
{
    float4 position : SV_Position;
    float3 normal : NORMAL;
    float3 color : COLOR;
    float4 lightVec[LIGHT_COUNT] : TEXCOORD0;
};

cbuffer VSConstants
{
    float4x4 projectionMatrix;
    float4x4 modelMatrix;
    float4 lightDir[LIGHT_COUNT];
};

struct LightStruct
{
    float4 lightPos[LIGHT_COUNT];
};

#if VK
[[vk::push_constant]]
LightStruct lights;
#else
cbuffer VSLights
{
    LightStruct lights;
};
#endif


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    output.normal = input.normal;
    output.color = input.color.rgb;

    float4x4 modelToProjection = mul(projectionMatrix, modelMatrix);
    output.position = mul(modelToProjection, float4(input.position, 1.0f));

    for (int i = 0; i < LIGHT_COUNT; ++i)
    {
        output.lightVec[i].xyz = lights.lightPos[i].xyz - input.position.xyz;
        output.lightVec[i].w = lights.lightPos[i].w;
    }

    return output;
}