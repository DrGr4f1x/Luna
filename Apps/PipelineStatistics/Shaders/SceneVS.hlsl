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
    float3 viewVec : TEXCOORD0;
    float3 lightVec : TEXCOORD1;
};


cbuffer VSConstants
{
    float4x4 projectionMatrix;
    float4x4 modelViewMatrix;
    float4 lightPos;
};


struct ObjectPosition
{
    float3 objPos;
};

#if VK
[[vk::push_constant]]
ObjectPosition position;
#else
cbuffer VSPositions
{
    ObjectPosition position;
};
#endif


VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;
    output.color = input.color.xyz;

    float3 worldPos = mul(modelViewMatrix, float4(input.pos + position.objPos, 1.0)).xyz;
    output.pos = mul(projectionMatrix, float4(worldPos, 1.0));

    float4 pos = mul(modelViewMatrix, float4(worldPos, 1.0));
    output.normal = mul((float3x3) modelViewMatrix, input.normal);
    output.lightVec = lightPos.xyz - pos.xyz;
    output.viewVec = -pos.xyz;
    return output;
}